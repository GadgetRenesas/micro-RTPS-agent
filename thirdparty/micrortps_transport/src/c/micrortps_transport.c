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
#include "micrortps_udp_transport.h"
#include <micrortps/transport/micrortps_transport.h>

// global variables

static locator_id_t g_loc_counter = 0;
static locator_id_plus_t g_loc_ids[MAX_NUM_LOCATORS];


// function declaration

locator_kind_t get_kind(const locator_id_t locator_id);
int extract_message(octet_t* out_buffer, const uint16_t buffer_len, buffer_t* internal_buffer);

locator_id_t add_udp_locator(const uint16_t local_udp_port, const uint16_t remote_udp_port, const uint8_t* remote_ip, micrortps_locator_t* const locator);
locator_id_t add_udp_locator_agent(const uint16_t local_port, micrortps_locator_t* const locator);
locator_id_t add_udp_locator_client(const uint16_t remote_port, const uint8_t* remote_ip, micrortps_locator_t* const locator);
locator_id_t add_serial_locator(const char* device, micrortps_locator_t* const locator);

int remove_locator(const locator_id_t locator_id);
int send_data(const octet_t* in_buffer, const size_t buffer_len, const locator_id_t locator_id);
int receive_data_timed(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms);
int receive_data(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id);

// function definition

locator_kind_t get_kind(const locator_id_t locator_id)
{
    for (uint8_t i = 0; i < g_loc_counter; ++i)
    {
        if (g_loc_ids[i].id == locator_id)
        {
            return g_loc_ids[i].kind;
        }
    }
    return LOC_NONE;
}

locator_id_t add_udp_locator(const uint16_t local_udp_port,
                             const uint16_t remote_udp_port, const uint8_t* remote_ip,
                             micrortps_locator_t* const locator)
{
    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# add_udp_locator(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    locator_id_t id = create_udp_locator(local_udp_port,
                                         remote_udp_port,
                                         remote_ip,
                                         ++g_loc_counter,
                                         locator);
    if (0 < id)
    {
        g_loc_ids[g_loc_counter - 1].id = id;
        g_loc_ids[g_loc_counter - 1].kind = locator->channel.kind;
    }
    else
    {
        --g_loc_counter;
    }

    return id;
}


locator_id_t add_udp_locator_agent(const uint16_t local_port, micrortps_locator_t* const locator)
{
    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# add_udp_locator_agent(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    return add_udp_locator(local_port, 0, NULL, locator);
}


locator_id_t add_udp_locator_client(const uint16_t remote_port, const uint8_t* remote_ip,
                                    micrortps_locator_t* const locator)
{
    if (NULL == locator || NULL == remote_ip)
    {
        MICRORTPS_TRANSPORT_PRINTF("# add_udp_locator_client(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    return add_udp_locator(0, remote_port, remote_ip, locator);
}


locator_id_t add_serial_locator(const char* device, micrortps_locator_t* const locator)
{
    if (NULL == device || 0 == strlen(device) || NULL ==  locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# add_serial_locator(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    locator_id_t id = create_serial_locator(device,
                                            ++g_loc_counter,
                                            locator);
    if (0 < id)
    {
        g_loc_ids[g_loc_counter - 1].id = id;
        g_loc_ids[g_loc_counter - 1].kind = locator->channel.kind;
    }
    else
    {
        --g_loc_counter;
    }

    return id;
}


/*locator_id_t add_locator(micrortps_locator_t* const locator)
{
    if (NULL == locator)
    {
        MICRORTPS_TRANSPORT_PRINTF("# add_locator_for_client(): BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    switch (locator->channel.kind)
    {
        case LOC_SERIAL:     return add_initializated_locator_serial   (locator);
        case LOC_UDP_AGENT:  return add_initializated_locator_udp_agent(locator);
        case LOC_UDP_CLIENT: return add_initializated_locator_udp_client(locator);

        default:
        break;
    }

    return MICRORTPS_TRANSPORT_ERROR;
}*/


int remove_locator(const locator_id_t locator_id)
{
    int ret = 0;
    switch (get_kind(locator_id))
    {
        case LOC_SERIAL:     ret = remove_serial_locator(locator_id); break;
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: ret = remove_udp_locator   (locator_id); break;

        default:             return MICRORTPS_TRANSPORT_ERROR;
    }

    // Remove reference
    if (MICRORTPS_TRANSPORT_OK == ret)
    {
        for (uint8_t i = 0; i < g_loc_counter; ++i)
        {
            if (g_loc_ids[i].id == locator_id)
            {
                g_loc_ids[i].id = 0;
                g_loc_ids[i].kind = LOC_NONE;
            }
        }
    }

    return ret;
}


int send_data(const octet_t* in_buffer, const size_t buffer_len, const locator_id_t locator_id)
{
    if (NULL == in_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    header_t header =
    {
        .marker = {'>', '>', '>'},
        .payload_len_h = 0u,
        .payload_len_l = 0u,
        .crc_h = 0u,
        .crc_l = 0u

    };

    // [>,>,>,topic_ID,seq,payload_length,CRCHigh,CRCLow,payload_start, ... ,payload_end]

    uint16_t crc = crc16(in_buffer, buffer_len);
    header.payload_len_h = (buffer_len >> 8) & 0xff;
    header.payload_len_l = buffer_len & 0xff;
    header.crc_h = (crc >> 8) & 0xff;
    header.crc_l = crc & 0xff;

    switch (get_kind(locator_id))
    {
        case LOC_SERIAL:     return send_serial(&header, in_buffer, buffer_len, locator_id);
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: return send_udp   (&header, in_buffer, buffer_len, locator_id);

        default:         return MICRORTPS_TRANSPORT_ERROR;
    }
}


int receive_data_timed(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id, const uint16_t timeout_ms)
{
    if (NULL == out_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    switch (get_kind(locator_id))
    {
        case LOC_SERIAL:     return receive_serial(out_buffer, buffer_len, locator_id, timeout_ms);
        case LOC_UDP_AGENT:
        case LOC_UDP_CLIENT: return receive_udp   (out_buffer, buffer_len, locator_id, timeout_ms);

        default:             return MICRORTPS_TRANSPORT_ERROR;
    }
}


int receive_data(octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id)
{
    return receive_data_timed(out_buffer, buffer_len, locator_id, DFLT_POLL_MS);
}


int extract_message(octet_t* out_buffer, const uint16_t buffer_len, buffer_t* internal_buffer)
{
    if (NULL == out_buffer || NULL == internal_buffer)
    {
        MICRORTPS_TRANSPORT_PRINTF("# BAD PARAMETERS!\n");
        return MICRORTPS_TRANSPORT_ERROR;
    }

    octet_t* rx_buffer = internal_buffer->buffer;
    uint16_t* rx_buff_pos = &(internal_buffer->buff_pos);

    // We read some
    uint16_t header_size = (uint16_t)sizeof(header_t);

    // but not enough
    if ((*rx_buff_pos) < header_size)
    {
        return 0;
    }

    uint16_t msg_start_pos = 0;

    for (msg_start_pos = 0; msg_start_pos <= (*rx_buff_pos) - header_size; ++msg_start_pos)
    {
        if ('>' == rx_buffer[msg_start_pos] && memcmp(rx_buffer + msg_start_pos, ">>>", 3) == 0)
        {
            break;
        }
    }

    // Start not found
    if (msg_start_pos > (*rx_buff_pos) - header_size)
    {
        MICRORTPS_TRANSPORT_PRINTF_ARGS("                                 (↓↓ %u)\n", msg_start_pos);
        // All we've checked so far is garbage, drop it - but save unchecked bytes
        memmove(rx_buffer, rx_buffer + msg_start_pos, (*rx_buff_pos) - msg_start_pos);
        (*rx_buff_pos) = (*rx_buff_pos) - msg_start_pos;
        return MICRORTPS_TRANSPORT_ERROR;
    }

    /*
     * [>,>,>,length_H,length_L,CRC_H,CRC_L,payloadStart, ... ,payloadEnd]
     */

    header_t* header = (header_t*) &rx_buffer[msg_start_pos];
    uint16_t payload_len = ((uint16_t) header->payload_len_h << 8) | header->payload_len_l;

    // The message won't fit the buffer.
    if ((uint16_t)buffer_len < header_size + payload_len)
    {
        return -EMSGSIZE;
    }

    // We do not have a complete message yet
    if (msg_start_pos + header_size + payload_len > (*rx_buff_pos))
    {
        // If there's garbage at the beginning, drop it
        if (msg_start_pos > 0)
        {
            MICRORTPS_TRANSPORT_PRINTF_ARGS("                                 (↓ %u)\n", msg_start_pos);
            memmove(rx_buffer, rx_buffer + msg_start_pos, (*rx_buff_pos) - msg_start_pos);
            (*rx_buff_pos) -= msg_start_pos;
        }

        return 0;
    }

    uint16_t read_crc = ((uint16_t) header->crc_h << 8) | header->crc_l;
    uint16_t calc_crc = crc16((uint8_t *) rx_buffer + msg_start_pos + header_size, payload_len);
    int ret = 0;

    if (read_crc != calc_crc)
    {
        MICRORTPS_TRANSPORT_PRINTF_ARGS("BAD CRC %u != %u\n", read_crc, calc_crc);
        MICRORTPS_TRANSPORT_PRINTF_ARGS("                                 (↓ %lu)\n", (unsigned long) (header_size + payload_len));
        ret = MICRORTPS_TRANSPORT_ERROR;

    }
    else
    {
        // copy message to outbuffer and set other return values
        memmove(out_buffer, rx_buffer + msg_start_pos + header_size, payload_len);
        ret = payload_len; // only payload, "+ header_size" for real size.
    }

    // discard message from rx_buffer
    (*rx_buff_pos) -= header_size + payload_len;
    memmove(rx_buffer, rx_buffer + msg_start_pos + header_size + payload_len, (*rx_buff_pos));

    return ret;
}
