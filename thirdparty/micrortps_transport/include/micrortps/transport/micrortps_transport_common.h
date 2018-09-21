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

#ifndef _MICRORTPS_TRANSPORT_COMMON_H_
#define _MICRORTPS_TRANSPORT_COMMON_H_


#include "micrortps_transport_dll.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <WinSock2.h>
#else
#ifdef UDP_ENABLED
#include <arpa/inet.h>
#endif // UDP_ENABLED
#include <netinet/in.h>
#endif // _WIN32


#define MICRORTPS_TRANSPORT_ERROR  -1
#define MICRORTPS_TRANSPORT_OK      0
#define RX_BUFFER_LENGTH            CONFIG_MAX_TRANSMISSION_UNIT_SIZE
#define UART_NAME_MAX_LENGTH        CONFIG_MAX_STRING_SIZE
#define IPV4_LENGTH                 4


#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MICRORTPS_TRANSPORT_LOG
#define MICRORTPS_TRANSPORT_LOG 0
#endif

#define MICRORTPS_TRANSPORT_PRINTF(fmt) \
            do { if (MICRORTPS_TRANSPORT_LOG) fprintf(stdout, fmt); } while (0)

#define MICRORTPS_TRANSPORT_PRINTF_ARGS(fmt, ...) \
            do { if (MICRORTPS_TRANSPORT_LOG) fprintf(stdout, fmt, __VA_ARGS__); } while (0)

transport_DllAPI void ms_sleep(int milliseconds);



#ifdef __cplusplus
}
#endif


typedef int8_t locator_id_t;

typedef uint8_t octet_t;

typedef enum LocatorKind
{
    LOC_NONE,
    LOC_SERIAL,
    LOC_UDP_AGENT,
    LOC_UDP_CLIENT

} locator_kind_t;

typedef struct
{
    octet_t buffer[RX_BUFFER_LENGTH];
    uint16_t buff_pos;

} buffer_t;


typedef struct
{
    int uart_fd;
    char uart_name[UART_NAME_MAX_LENGTH];
    uint32_t baudrate;

} serial_channel_t;


typedef struct
{
#ifdef _WIN32
    SOCKET socket_fd;
#else
    int socket_fd;
#endif

    uint16_t local_udp_port;
    uint16_t remote_udp_port;

    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

} udp_channel_t;


typedef union TransportChannelU
{
    serial_channel_t serial;
    udp_channel_t udp;

} transport_channel_u_t;


typedef struct TransportChannel
{
    locator_kind_t kind;
    transport_channel_u_t _;

} transport_channel_t;


typedef struct micrortps_locator_t
{
    buffer_t rx_buffer;
    uint8_t locator_id;
    uint8_t idx;
    bool open;
    uint32_t poll_ms;

    transport_channel_t channel;

} micrortps_locator_t;

#endif
