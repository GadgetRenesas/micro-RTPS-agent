// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef _PROXY_CLIENT_H
#define _PROXY_CLIENT_H

#include <agent/MessageHeader.h>
#include <agent/datareader/DataReader.h>
#include <agent/datawriter/DataWriter.h>
#include <agent/client/StreamsManager.h>

#include <array>
#include <map>

namespace eprosima {
namespace micrortps {

class ProxyClient : public ReaderListener
{
public:
    ProxyClient() = default;

    /**
     * @brief Creates a ProxyClient from a given object representation.
     *
     * @param client
     * @param header
     */
    ProxyClient(dds::xrce::CLIENT_Representation client,
                const dds::xrce::ClientKey& client_key,
                const dds::xrce::SessionId& session_id,
                uint32_t addr, uint16_t port);

    ~ProxyClient();

    /**
     * @brief Deletes the copy constructor.
     */
    ProxyClient(const ProxyClient&) = delete;

    /**
     * @brief Default move constructor.
     *
     * @param x Object to move.
     */
    ProxyClient(ProxyClient&& x) noexcept;

    /**
     * @brief Deletes the assignment operator.
     */
    ProxyClient& operator=(const ProxyClient&) = delete;

    /**
     * @brief Default move assignment operator.
     *
     * @param x Object to move.
     * @return New reference where the object is moved.
     */
    ProxyClient& operator=(ProxyClient&& x) noexcept;

    /**
     * @brief This operation attempts to create a XRCE object according to the specification provided in the
     *        object_representation paramenter.
     *
     * @param creation_mode          Controls the behavior of the operation when there is an existing object that
     *                               partially matches the description of the object that the client wants to create.
     * @param objectid_prefix        Configures the desired ObjectId for the created object.
     * @param object_representation  A representation of the object that the client wants to create.
     *
     * @return
     */
    dds::xrce::ResultStatus create(const dds::xrce::CreationMode& creation_mode,
                                   const dds::xrce::ObjectPrefix& objectid_prefix,
                                   const dds::xrce::ObjectVariant& object_representation);

    /**
     * @brief This operation deletes an existing object.
     *
     * @param object_id
     *
     * @return The return values are the following:
     *         * Íf the object is successfully deleted STATUS_OK.
     *         * If the object does not exist STATUS_ERR_UNKNOWN_REFERENCE.
     */
    dds::xrce::ResultStatus delete_object(const dds::xrce::ObjectId& object_id);

    /**
     * @brief This operation updates a given XRCE Object.
     *
     * @param object_id       The object being updated.
     * @param representation  The variant of the updated object.
     *
     * @return Indicates whether the operation succeeded and the current status of the object.
     */
    dds::xrce::ResultStatus update(const dds::xrce::ObjectId& object_id,
                                   const dds::xrce::ObjectVariant& representation);

    /**
     * @brief This operation returns the configuration and activity data for an existing object.
     *
     * @param object_id The object queried.
     *
     * @return Indicates whether the operation succeeded.
     */
    dds::xrce::ObjectInfo get_info(const dds::xrce::ObjectId& object_id);

    /**
     * @brief Callback called on new data from a given DataReader.
     *
     * @param stream_id   The stream used to send data to the Client.
     * @param object_id   The DataReader id.
     * @param request_id  The request id performed by the Client.
     *
     * @param buffer
     */
    void on_read_data(const dds::xrce::StreamId& stream_id,
                      const dds::xrce::ObjectId& object_id,
                      const dds::xrce::RequestId& request_id,
                      const std::vector<unsigned char>& buffer);

    /**
     * @brief This function return a given XRCE Object.
     *
     * @param object_id The object queried.
     *
     * @return A pointer to the XRCE Object.
     */
    XRCEObject* get_object(const dds::xrce::ObjectId& object_id);

    /**
     * @brief This function return the Client's key.
     *
     * @return The Client's key.
     */
    const dds::xrce::ClientKey& get_client_key() const { return client_key_; }

    /**
     * @brief This function return the Client's session id.
     *
     * @return The Client's session id.
     */
    const dds::xrce::SessionId& get_session_id() const { return session_id_; }

    /**
     * @brief This function return the Client's address.
     *
     * @return The Client's address.
     */
    uint32_t get_addr() const { return addr_; }

    /**
     * @brief This function return the Client's port.
     *
     * @return The Client's port.
     */
    uint16_t get_port() const { return port_; }

    /**
     * @brief This function return a stream manager used by the ProxyClient.
     *
     * @return The stream manager of the ProxyClient.
     */
    StreamsManager& stream_manager();

private:
    dds::xrce::CLIENT_Representation representation_;

    std::mutex objects_mutex_;
    std::map<dds::xrce::ObjectId, std::unique_ptr<XRCEObject>> objects_;

    dds::xrce::ClientKey client_key_;
    dds::xrce::SessionId session_id_;
    uint32_t addr_;
    uint16_t port_;

    StreamsManager streams_manager_;

    bool create(const dds::xrce::ObjectId& object_id, const dds::xrce::ObjectVariant& representation);
};

} // namespace micrortps
} // namespace eprosima

#endif //_PROXY_CLIENT_H
