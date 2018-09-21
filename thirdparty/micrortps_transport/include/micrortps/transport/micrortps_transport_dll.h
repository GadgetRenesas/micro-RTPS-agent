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

#ifndef _MICRORTPS_TRANSPORT_DLL_H_
#define _MICRORTPS_TRANSPORT_DLL_H_

#include <micrortps/transport/config.h>

#if defined(_WIN32)
#if defined(micrortps_transport_SHARED)
#if defined(micrortps_transport_EXPORTS)
#define transport_DllAPI __declspec( dllexport )
#else
#define transport_DllAPI __declspec( dllimport )
#endif // transport_EXPORTS
#else
#define transport_DllAPI
#endif // BUILDING_SHARED_LIBS
#else
#define transport_DllAPI
#endif // _WIN32

#endif // _MICRORTPS_TRANSPORT_DLL_H_
