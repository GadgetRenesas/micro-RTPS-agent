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

/**
 * @file DataWriter.h
 *
 */

#ifndef DATAWRITER_H_
#define DATAWRITER_H_

#include <string>

#include <agent/XRCEObject.hpp>
#include <agent/types/TopicPubSubType.h>

namespace eprosima {

namespace fastrtps {
class Participant;
class Publisher;
} // namespace fastrtps

namespace micrortps {

class WRITE_DATA_Payload;
/**
 * Class DataWriter, used to send data to associated datareaders.
 * @ingroup MICRORTPS_MODULE
 */
class DataWriter : public XRCEObject
{
  public:
    DataWriter(const dds::xrce::ObjectId& id,
               Participant& rtps_participant,
               const std::string& profile_name = "");
    ~DataWriter() override;

    DataWriter(DataWriter&&)      = delete;
    DataWriter(const DataWriter&) = delete;
    DataWriter& operator=(DataWriter&&) = delete;
    DataWriter& operator=(const DataWriter&) = delete;

    bool init();
    bool init(const std::string& xmlrep);
    const dds::xrce::ResultStatus& write(dds::xrce::DataRepresentation& data);
    bool write(dds::xrce::WRITE_DATA_Payload_Data& write_data);

  private:
    fastrtps::Participant& mp_rtps_participant;
    fastrtps::Publisher* mp_rtps_publisher;
    std::string m_rtps_publisher_prof;
    TopicPubSubType topic_type_;
    dds::xrce::ResultStatus result_status_;

    bool check_registered_topic(const std::string& topic_data_type) const;
};

} /* namespace micrortps */
} /* namespace eprosima */

#endif /* DATAWRITER_H_ */
