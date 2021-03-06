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

#ifndef _MICRORTPS_UDP_TRANSPORT_H_
#define _MICRORTPS_UDP_TRANSPORT_H_

#include "micrortps_transport_common_priv.h"

#ifdef __cplusplus
extern "C"
{
#endif


locator_id_t         create_udp_locator (uint16_t local_udp_port,
                                         uint16_t remote_udp_port, const uint8_t* remote_ip,
                                         locator_id_t locator_id, micrortps_locator_t* const locator);
int                  remove_udp_locator (const locator_id_t locator_id);
int                  open_udp_locator   (micrortps_locator_t* const locator);
int                  close_udp_locator  (micrortps_locator_t* const locator);
int                  send_udp           (const header_t* header, const octet_t* in_buffer, const size_t length,
                                         const locator_id_t locator_id);
int                  receive_udp        (octet_t* out_buffer, const size_t buffer_len, const locator_id_t locator_id,
                                         const uint16_t timeout_ms);
micrortps_locator_t* get_udp_channel    (const locator_id_t locator_id);


#ifdef __cplusplus
}
#endif

#endif
