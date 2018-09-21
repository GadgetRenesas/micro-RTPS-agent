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

#include "micrortps_udp_transport.h"

#ifdef UDP_ENABLED

static micrortps_locator_t* g_udp_locators[MAX_NUM_LOCATORS];
static uint8_t g_num_locators = 0;
static struct pollfd g_poll_fds[MAX_NUM_LOCATORS];

#ifdef _WIN32
static WSADATA wsa;
#endif

#endif


/// extern function declarations

uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(const uint8_t* buffer, size_t len);
int extract_message(octet_t* out_buffer, const size_t buffer_len, buffer_t* internal_buffer);


/// local function declarations

locator_id_t         create_udp_locator (uint16_t local_udp_port, uint16_t remote_udp_port, const uint8_t* remote_ip, locator_id_t loc_id, micrortps_locator_t* const locator);
int                  remove_udp_locator (const locator_id_t locator_id);
int                  init_udp_locator   (micrortps_locator_t* const locator);
int                  open_udp_locator   (micrortps_locator_t* const locator);
int                  close_udp_locator  (micrortps_locator_t* const locator);
int                  write_udp          (const void* buffer, const size_t len, micrortps_locator_t* const locator);
int                  read_udp           (micrortps_locator_t* const locator);
int                  send_udp           (const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id);
int                  receive_udp        (octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms);
micrortps_locator_t* get_udp_locator    (const locator_id_t locator_id);


/// function definition

micrortps_locator_t* get_udp_locator(const locator_id_t locator_id)
{
#ifdef UDP_ENABLED

    micrortps_locator_t* ret = NULL;
    for (int i = 0; i < g_num_locators; ++i)
    {
        if (NULL != g_udp_locators[i] &&
            g_udp_locators[i]->locator_id == locator_id)
        {
            ret = g_udp_locators[i];
            break;
        }
    }
    return ret;

#else

    return NULL;

#endif // UDP_ENABLED
}


locator_id_t create_udp_locator(uint16_t local_udp_port,
                        uint16_t remote_udp_port, const uint8_t* remote_ip,
                        locator_id_t loc_id, micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if (0 >= loc_id || NULL == locator || MAX_NUM_LOCATORS <= g_num_locators)
    {
        MICRORTPS_TRANSPORT_PRINTF("# create_udp_locator() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    /// Fill locator struct

    memset(locator, 0, sizeof(micrortps_locator_t));
    locator->rx_buffer.buff_pos = 0;
    locator->open = false;
    locator->poll_ms = DFLT_POLL_MS;
    locator->channel.kind = LOC_UDP_AGENT;

    udp_channel_t* channel = &locator->channel._.udp;
#ifdef _WIN32
    channel->socket_fd = INVALID_SOCKET;
#else
    channel->socket_fd = -1;
#endif

    // local
    channel->local_udp_port = local_udp_port;
    channel->local_addr.sin_port = htons(channel->local_udp_port);
    channel->local_addr.sin_family = AF_INET;
    channel->local_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 - To use whatever interface (IP) of the system

    // remote
    channel->remote_udp_port = remote_udp_port;
    channel->remote_addr.sin_family = AF_INET;
    if (NULL != remote_ip)
    {
        locator->channel.kind = LOC_UDP_CLIENT;
        memcpy((void*) &channel->remote_addr.sin_addr, remote_ip, IPV4_LENGTH);
        channel->remote_addr.sin_port = htons(channel->remote_udp_port);
    }

    locator->locator_id = (uint8_t)~0;
    for (uint8_t i = 0; i < MAX_NUM_LOCATORS; ++i)
    {
        if (NULL == g_udp_locators[i])
        {
            locator->locator_id = loc_id;
            locator->idx = i;
            g_udp_locators[i] = locator;
            ++g_num_locators;
            break;
        }
    }

    #ifdef TRANSPORT_LOGS
    printf("> Create udp locator id: %d\n", locator->locator_id);
    #endif // TRANSPORT_LOGS

    return locator->locator_id;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int init_udp_locator(micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    udp_channel_t* channel = &locator->channel._.udp;

#ifdef _WIN32

    //Initialise winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        MICRORTPS_TRANSPORT_PRINTF_ARGS("Failed initialising Winsock, error code: %d\n",WSAGetLastError());
        return MICRORTPS_TRANSPORT_ERROR;
    }

#endif

#ifdef _WIN32
    if (INVALID_SOCKET == (channel->socket_fd = socket(AF_INET, SOCK_DGRAM, 0)))
#else

    if (-1 == (channel->socket_fd = socket(AF_INET, SOCK_DGRAM, 0)))
#endif
    {
        MICRORTPS_TRANSPORT_PRINTF("# create socket failed\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    if (0 == channel->remote_addr.sin_addr.s_addr) // AGENT
    {
        if (0 > bind(channel->socket_fd, (struct sockaddr *)&channel->local_addr, sizeof(channel->local_addr)))
        {
#ifdef _WIN32
            MICRORTPS_TRANSPORT_PRINTF_ARGS("# bind failed, socket_fd: %llu port: %u\n", channel->socket_fd, channel->local_udp_port);
#else
            MICRORTPS_TRANSPORT_PRINTF_ARGS("# bind failed, socket_fd: %d port: %u\n", channel->socket_fd, channel->local_udp_port);
#endif
            return MICRORTPS_TRANSPORT_ERROR;
        }

        #ifdef TRANSPORT_LOGS
        printf("> Agent locator initialized on port %d\n", channel->local_udp_port);
        #endif
    }
    else // CLIENT
    {
        #ifdef TRANSPORT_LOGS
        printf("> Client initialized on port %d\n", channel->local_udp_port);
        #endif
    }

    return MICRORTPS_TRANSPORT_OK;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int remove_udp_locator(const locator_id_t loc_id)
{
#ifdef UDP_ENABLED

    micrortps_locator_t* locator = get_udp_locator(loc_id);
    if (NULL == locator)
    {
        return MICRORTPS_TRANSPORT_ERROR;
    }

    if (locator->open)
    {
        close_udp_locator(locator);
    }
    g_udp_locators[locator->idx] = NULL;

    return MICRORTPS_TRANSPORT_OK;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int open_udp_locator(micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if (NULL == locator || 0 > init_udp_locator(locator))
    {
        MICRORTPS_TRANSPORT_PRINTF("# open_udp() -> BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    #ifdef TRANSPORT_LOGS
    printf("> UDP channel opened\n");
    #endif

    locator->open = true;
    g_poll_fds[locator->idx].fd = locator->channel._.udp.socket_fd;
    g_poll_fds[locator->idx].events = POLLIN;
    return MICRORTPS_TRANSPORT_OK;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int close_udp_locator(micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    udp_channel_t* channel = &locator->channel._.udp;

#ifdef _WIN32
    if (INVALID_SOCKET != channel->socket_fd)
    {
        #ifdef TRANSPORT_LOGS
        printf("> Close socket\n");
        #endif

        shutdown(channel->socket_fd, SD_BOTH);
        closesocket(channel->socket_fd);
        WSACleanup();

        channel->socket_fd = INVALID_SOCKET;
    }
#else
    if (-1 != channel->socket_fd)
    {
        #ifdef TRANSPORT_LOGS
        printf("> Close socket\n");
        #endif

        shutdown(channel->socket_fd, SHUT_RDWR);
        close(channel->socket_fd);

        channel->socket_fd = -1;
    }
#endif

    locator->open = false;
    memset(&g_poll_fds[locator->idx], 0, sizeof(struct pollfd));
    return MICRORTPS_TRANSPORT_OK;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int read_udp(micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if ( NULL == locator      ||
        (!locator->open && 0 > open_udp_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# Error read UDP channel\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    udp_channel_t* channel = &locator->channel._.udp;

    // TODO: for several channels this can be optimized
    int ret = 0;
    #ifdef _WIN32
    int addrlen = sizeof(channel->remote_addr);
    int r = WSAPoll(&g_poll_fds[locator->idx], 1, locator->poll_ms);
    #else
    static socklen_t addrlen = sizeof(channel->remote_addr);
    int r = poll(&g_poll_fds[locator->idx], 1, locator->poll_ms);
    #endif

    if (r > 0 && (g_poll_fds[locator->idx].revents & POLLIN))
    {
        ret = recvfrom(channel->socket_fd,
                       (void *) (locator->rx_buffer.buffer + locator->rx_buffer.buff_pos),
                       sizeof(locator->rx_buffer.buffer) - locator->rx_buffer.buff_pos,
                       0,
                       (struct sockaddr *) &channel->remote_addr,
                       &addrlen);
    }

    return ret;

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}


int receive_udp(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms)
{
#ifdef UDP_ENABLED

    if (NULL == out_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# receive_udp(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    micrortps_locator_t* locator = get_udp_locator(locator_id);
    if (NULL == locator ||
        (!locator->open && 0 > open_udp_locator(locator)))
    {
        return MICRORTPS_TRANSPORT_ERROR;
    }

    locator->poll_ms = timeout_ms;

    int len = read_udp(locator);
    if (len <= 0)
    {
        int errsv = errno;

        if (errsv && EAGAIN != errsv && ETIMEDOUT != errsv)
        {
//            MICRORTPS_TRANSPORT_PRINTF("# Read fail %d\n", errsv);
        }
    }
    else
    {
        // We read some bytes, trying extract a whole message
        locator->rx_buffer.buff_pos += (uint16_t)len;
    }

    return extract_message(out_buffer, buffer_len, &locator->rx_buffer);

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int write_udp(const void* buffer, const size_t len, micrortps_locator_t* const locator)
{
#ifdef UDP_ENABLED

    if ( NULL == buffer       ||
         NULL == locator      ||
        (!locator->open && 0 > open_udp_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# Error write UDP channel\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    udp_channel_t* channel = &locator->channel._.udp;
    if (0 == channel->remote_addr.sin_addr.s_addr)
    {
        MICRORTPS_TRANSPORT_PRINTF("# Error write UDP channel, do not exist a send address\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

#ifdef _WIN32
    return sendto(channel->socket_fd, buffer, (int)len, 0, (struct sockaddr *)&channel->remote_addr, sizeof(channel->remote_addr));
#else
    return sendto(channel->socket_fd, buffer, len, 0, (struct sockaddr *)&channel->remote_addr, sizeof(channel->remote_addr));
#endif

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}

int send_udp(const header_t* header, const octet_t* in_buffer, const size_t length, const locator_id_t locator_id)
{
#ifdef UDP_ENABLED

    if (NULL == in_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    micrortps_locator_t* locator = get_udp_locator(locator_id);
    if ( NULL == locator      ||
        (!locator->open && 0 > open_udp_locator(locator)))
    {
        MICRORTPS_TRANSPORT_PRINTF("# Error send UDP channel\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    int len = write_udp(header, sizeof(header_t), locator);
    if (len != sizeof(header_t))
    {
        return len;
    }

    len = write_udp(in_buffer, length, locator);
    if ((unsigned int)len != length)
    {
        return len;
    }

    return len; // only payload, + sizeof(header); for real size.

#else

    return MICRORTPS_TRANSPORT_ERROR;

#endif // UDP_ENABLED
}
