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

#ifndef _MICRORTPS_TRANSPORT_COMMON_PRIV_H_
#define _MICRORTPS_TRANSPORT_COMMON_PRIV_H_

#include <micrortps/transport/micrortps_transport_common.h>
#include <micrortps/transport/micrortps_transport_dll.h>

#include <string.h>

#ifdef _WIN32

#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <WinBase.h>

#else

#include <unistd.h>
#include <termios.h>
#include <poll.h>

#ifdef UDP_ENABLED

#include <arpa/inet.h>

#endif // UDP_ENABLED

#endif // _WIN32


#define DFLT_UDP_PORT               2019
#define DFLT_UART                 "/dev/ttyACM0"
#define DFLT_BAUDRATE             115200
#define DFLT_POLL_MS                  20
#define MAX_NUM_LOCATORS          CONFIG_MAX_NUM_LOCATORS
#define MAX_PENDING_CONNECTIONS       10



#ifdef __cplusplus
extern "C"
{
#endif



uint16_t crc16_byte(uint16_t crc, const uint8_t data);
uint16_t crc16(const uint8_t* buffer, size_t len);
void print_buffer(const uint8_t* buffer, const size_t len);



#ifdef __cplusplus
}
#endif

#if defined(_WIN32)
#define __PACKED__(struct_to_pack) __pragma(pack(push, 1)) struct_to_pack __pragma(pack(pop))
#else
#define __PACKED__(struct_to_pack) struct_to_pack __attribute__((__packed__))
#endif

__PACKED__( struct Header
{
    char marker[3];
    octet_t payload_len_h;
    octet_t payload_len_l;
    octet_t crc_h;
    octet_t crc_l;
});

typedef struct Header header_t;


__PACKED__( struct Locator
{
    locator_id_t id;
    locator_kind_t kind;
    octet_t data[16];
});

typedef struct Locator locator_t;

__PACKED__( struct Locator_id_plus
{
    locator_id_t id;
    locator_kind_t kind;
});

typedef struct Locator_id_plus locator_id_plus_t;


#endif
