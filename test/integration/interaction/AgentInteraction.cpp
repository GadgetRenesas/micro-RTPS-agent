#include "AgentInteraction.hpp"
#include <agent/Root.h>

#define AGENT_MAX_NUM_ATTEMPTS 100
#define AGENT_MAX_TIME_WAIT 10

using namespace eprosima::micrortps;

AgentT::AgentT(uint16_t port)
    : agent_(&root()),
      port_(port)
{
}

void AgentT::launch()
{
    agent_->init(port_);
    thread_ = std::thread(&Agent::run, agent_);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void AgentT::stop()
{
    agent_->stop();
    thread_.join();
    agent_->abort_execution();
}

bool AgentT::client_disconnected(const std::array<uint8_t, 4> client_key)
{
    return  agent_->get_client(client_key) == nullptr;
}

bool AgentT::client_connected(const std::array<uint8_t, 4> client_key)
{
    ProxyClient* client = nullptr;
    for (uint16_t i = 0; i < AGENT_MAX_NUM_ATTEMPTS && client == nullptr; ++i)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(AGENT_MAX_TIME_WAIT));
        client = agent_->get_client(client_key);
    }
    return client != nullptr;
}

bool AgentT::object_created(const std::array<uint8_t, 4> client_key, const std::array<uint8_t, 2> object_id)
{
    XRCEObject* object = nullptr;
    ProxyClient* client = agent_->get_client(client_key);
    if (nullptr != client)
    {
        for (uint16_t i = 0; i < AGENT_MAX_NUM_ATTEMPTS; ++i)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(AGENT_MAX_TIME_WAIT));
            object = client->get_object(object_id);
        }
    }
    return object != nullptr;
}

bool AgentT::object_deleted(const std::array<uint8_t, 4> client_key, const std::array<uint8_t, 2> object_id)
{
    XRCEObject* object = nullptr;
    ProxyClient* client = agent_->get_client(client_key);
    if (nullptr != client)
    {
        for (uint16_t i = 0; i < AGENT_MAX_NUM_ATTEMPTS; ++i)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(AGENT_MAX_TIME_WAIT));
            object = client->get_object(object_id);
        }
    }
    return (object == nullptr) && (client != nullptr);
}

