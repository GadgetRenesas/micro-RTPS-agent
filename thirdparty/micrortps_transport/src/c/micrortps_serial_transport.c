// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "micrortps_serial_transport.h"

static micrortps_locator_t* g_serial_locators[MAX_NUM_LOCATORS] = {0};
static uint8_t g_num_locators = 0;

#ifndef _WIN32

static struct pollfd g_poll_fds[MAX_NUM_LOCATORS] = {0};

#endif // _WIN32


/// extern function declarations

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(const uint8_t* buffer, size_t len);
int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);


/// local function declarations

locator_id_t         create_serial_locator (const char* device, locator_id_t locator_id, micrortps_locator_t* const locator);
int                  remove_serial_locator (const locator_id_t locator_id);
int                  open_serial_locator   (micrortps_locator_t* const locator);
int                  close_serial_locator  (micrortps_locator_t* const locator);
int                  write_serial          (const void* buffer, const size_t len, micrortps_locator_t* const locator);
int                  read_serial           (micrortps_locator_t* const channel);
int                  send_serial           (const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id);
int                  receive_serial        (octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms);
micrortps_locator_t* get_serial_locator    (const locator_id_t locator_id);


/// function definition

micrortps_locator_t* get_serial_locator(const locator_id_t locator_id)
{
    micrortps_locator_t* ret = NULL;

#ifdef _WIN32

    (void) locator_id;
    return ret;

#else

    for (int i = 0; i < g_num_locators; ++i)
    {
        if (NULL != g_serial_locators[i] &&
            g_serial_locators[i]->locator_id == locator_id)
        {
            ret = g_serial_locators[i];
            break;
        }
    }

    return ret;

#endif
}


locator_id_t create_serial_locator(const char* device, locator_id_t loc_id, micrortps_locator_t* const locator)
{

#ifdef _WIN32

    (void) device;
    (void) loc_id;
    (void) locator;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == device || 0 == strlen(device) || UART_NAME_MAX_LENGTH <= strlen(device) ||
        NULL == locator || MAX_NUM_LOCATORS <= g_num_locators)
    {
        MICRORTPS_TRANSPORT_PRINTF("# create_serial_locator() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }


    /// Fill locator struct

    memset(locator, 0, sizeof(micrortps_locator_t));
    locator->rx_buffer.buff_pos = 0;
    locator->open = false;
    locator->poll_ms = DFLT_POLL_MS;
    locator->channel.kind = LOC_SERIAL;
    serial_channel_t* channel = &locator->channel._.serial;
    memcpy(channel->uart_name, device, UART_NAME_MAX_LENGTH);
    channel->uart_fd = -1;
    channel->baudrate = DFLT_BAUDRATE;

    locator->locator_id = -1;
    for (int i = 0; i < MAX_NUM_LOCATORS; ++i)
    {
        if (NULL == g_serial_locators[i])
        {
            locator->locator_id = loc_id;
            locator->idx = i;
            g_serial_locators[i] = locator;
            ++g_num_locators;
            break;
        }
    }

    #ifdef TRANSPORT_LOGS
    printf("> Create serial locator id: %d\n", locator->locator_id);
    #endif // TRANSPORT_LOGS

    return locator->locator_id;

#endif
}

int remove_serial_locator(const locator_id_t loc_id)
{
#ifdef _WIN32

    (void) loc_id;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    micrortps_locator_t* locator = get_serial_locator(loc_id);
    if (NULL == locator)
    {
        return MICRORTPS_TRANSPORT_ERROR;
    }

    if (locator->open)
    {
        close_serial_locator(locator);
    }
    g_serial_locators[locator->idx] = NULL;

    return MICRORTPS_TRANSPORT_OK;

#endif
}

int open_serial_locator(micrortps_locator_t* const locator)
{

#ifdef _WIN32

    (void) locator;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == locator)
    {
        return MICRORTPS_TRANSPORT_ERROR;
    }

    serial_channel_t* channel = &locator->channel._.serial;

    // Open a serial port
    channel->uart_fd = open(channel->uart_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (channel->uart_fd < 0)
    {
        MICRORTPS_TRANSPORT_PRINTF_ARGS("failed to open device: %s (%d)\n", channel->uart_name, errno);
        return -errno;
    }

    // Try to set baud rate
    struct termios uart_config;
    int termios_state;

    // Back up the original uart configuration to restore it after exit
    if ((termios_state = tcgetattr(channel->uart_fd, &uart_config)) < 0)
    {
        int errno_bkp = errno;
        MICRORTPS_TRANSPORT_PRINTF_ARGS("ERR GET CONF %s: %d (%d)\n", channel->uart_name, termios_state, errno);
        close_serial_locator(locator);
        return -errno_bkp;
    }

    // Clear ONLCR flag (which appends a CR for every LF)
    cfsetospeed(&uart_config, (speed_t)channel->baudrate);
    cfsetispeed(&uart_config, (speed_t)channel->baudrate);

    uart_config.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    uart_config.c_cflag &= ~CSIZE;
    uart_config.c_cflag |= CS8;         /* 8-bit characters */
    uart_config.c_cflag &= ~PARENB;     /* no parity bit */
    uart_config.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    uart_config.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    uart_config.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    uart_config.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    uart_config.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    uart_config.c_cc[VMIN] = 1;
    uart_config.c_cc[VTIME] = 1;

    if ((termios_state = tcsetattr(channel->uart_fd, TCSANOW, &uart_config)) < 0)
    {
        int errno_bkp = errno;
        MICRORTPS_TRANSPORT_PRINTF_ARGS("ERR SET CONF %s (%d)\n", channel->uart_name, errno);
        close_serial_locator(locator);
        return -errno_bkp;
    }

    uint8_t aux_buffer[64];
    while (0 < read(channel->uart_fd, (void *)&aux_buffer, 64)); //clear previous data

    locator->open = true;
    g_poll_fds[locator->idx].fd = channel->uart_fd;
    g_poll_fds[locator->idx].events = POLLIN;

    return channel->uart_fd;

#endif
}

int close_serial_locator(micrortps_locator_t* const locator)
{

#ifdef _WIN32

    (void) locator;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# close_serial_locator() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    serial_channel_t* channel = &locator->channel._.serial;

    if (0 != close(channel->uart_fd))
    {
        MICRORTPS_TRANSPORT_PRINTF("# close_serial_locator() -> close error\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    channel->uart_fd = -1;
    locator->open = false;
    memset(&g_poll_fds[locator->idx], 0, sizeof(struct pollfd));

    #ifdef TRANSPORT_LOGS
    printf("> Close UART %s\n", locator->channel._.serial.uart_name);
    #endif // TRANSPORT_LOGS

    return MICRORTPS_TRANSPORT_OK;

#endif
}

int read_serial(micrortps_locator_t* const locator)
{

#ifdef _WIN32

    (void) locator;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == locator ||
        (!locator->open && 0 > open_serial_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# read_serial() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    // TODO: for several channels this can be optimized
    int ret = 0;
    int r = poll(g_poll_fds, g_num_locators, locator->poll_ms);
    if (r > 0 && (g_poll_fds[locator->idx].revents & POLLIN))
    {
        ret = read(locator->channel._.serial.uart_fd,
                   (void *) (locator->rx_buffer.buffer + locator->rx_buffer.buff_pos), // buffer pos
                   sizeof(locator->rx_buffer.buffer) - locator->rx_buffer.buff_pos);   // len
    }

    return ret;

#endif
}


int receive_serial(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms)
{

#ifdef _WIN32

    (void) out_buffer;
    (void) buffer_len;
    (void) locator_id;
    (void) timeout_ms;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == out_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# receive_serial() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    micrortps_locator_t* locator = get_serial_locator(locator_id);
    if (NULL == locator ||
        (!locator->open && 0 > open_serial_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# receive_serial() -> error, serial not open\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    locator->poll_ms = timeout_ms;

    int len = read_serial(locator);
    if (len <= 0)
    {
        int errsv = errno;

        if (errsv && EAGAIN != errsv && ETIMEDOUT != errsv)
        {
//            MICRORTPS_TRANSPORT_PRINTF("Read fail %d\n", errsv);
        }
    }
    else
    {
        // We read some bytes, trying extract a whole message
        locator->rx_buffer.buff_pos += len;
    }

    return extract_message(out_buffer, buffer_len, &locator->rx_buffer);

#endif
}

int write_serial(const void* buffer, const size_t len, micrortps_locator_t* const locator)
{

#ifdef _WIN32

    (void) buffer;
    (void) len;
    (void) locator;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == buffer       ||
        NULL == locator      ||
        (!locator->open && 0 > open_serial_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# write_serial() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    return (int)write(locator->channel._.serial.uart_fd, buffer, len);

#endif
}

int send_serial(const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id)
{

#ifdef _WIN32

    (void) header;
    (void) in_buffer;
    (void) length;
    (void) locator_id;
    return MICRORTPS_TRANSPORT_ERROR;

#else

    if (NULL == in_buffer)
    {
        return MICRORTPS_TRANSPORT_ERROR;
    }

    micrortps_locator_t* locator = get_serial_locator(locator_id);
    if (NULL == locator      ||
        (!locator->open && 0 > open_serial_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# send_serial() -> error, serial not open\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    int len = write_serial(header, sizeof(header_t), locator);
    if (len != sizeof(header_t))
    {
        return len;
    }

    len = write_serial(in_buffer, length, locator);

    return len; // only payload, + sizeof(header); for real size.

#endif
}
