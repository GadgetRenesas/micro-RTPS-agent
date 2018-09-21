#include <micrortps/transport/micrortps_transport.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>

#define MAX_NUM_ATTEMPS 1000
#define MAX_TIMEOUT     100

/****************************************************************************************
 * Server
 ****************************************************************************************/
class Server
{
public:
    Server(uint16_t port) : port_(port) {}
    ~Server()
    {
        remove_locator(locator_.locator_id);
    }

    bool init()
    {
        running_cond_ = true;
        locator_id_t loc_id = add_udp_locator_agent(port_, &locator_);
        return (loc_id != MICRORTPS_TRANSPORT_ERROR);
    }
    void run()
    {
        octet_t buffer[32] ={0};
        for (uint16_t i = 0; i < MAX_NUM_ATTEMPS && running_cond_; ++i)
        {
            if (0 < receive_data_timed(buffer, sizeof(buffer), locator_.locator_id, MAX_TIMEOUT))
            {
                if (0 == strcmp(client_key_, (char*)buffer))
                {
                    send_data((octet_t*)key_, strlen(key_), locator_.locator_id);
                }
            }
        }
    }
    void stop() { running_cond_ = false; }
    const char* get_key() const { return key_; }

private:
    micrortps_locator_t locator_;
    const uint16_t port_;
    const char key_[16] = "TokcRgmVPGYk4i2";
    const char client_key_[16] = "SFvHsNFyyvOC2I0";
    static std::atomic<bool> running_cond_;
};

std::atomic<bool> Server::running_cond_{true};

/****************************************************************************************
 * Client
 ****************************************************************************************/
class Client
{
public:
    Client(const uint8_t ip[4], uint16_t port)
        : ip_{ip[0], ip[1], ip[2], ip[3]},
          port_(port)
    {}
    ~Client()
    {
        remove_locator(locator_.locator_id);
    }

    bool connect()
    {
        running_cond_ = true;
        locator_id_t loc_id = add_udp_locator_client(port_, ip_, &locator_);
        return (loc_id != MICRORTPS_TRANSPORT_ERROR);
    }
    void run()
    {
        octet_t buffer[32] = {11};
        for (uint16_t i = 0; i < MAX_NUM_ATTEMPS && running_cond_; ++i)
        {
            send_data((octet_t*)key_, strlen(key_), locator_.locator_id);
            int len = receive_data_timed(buffer, sizeof(buffer), locator_.locator_id, MAX_TIMEOUT);
            if (0 < len && MICRORTPS_TRANSPORT_ERROR != len)
            {
                memcpy(key_received_, buffer, len);
                running_cond_ = false;
            }
        }
    }
    const char* get_key() const { return key_received_; }

private:
    micrortps_locator_t locator_;
    const uint8_t ip_[4];
    const uint16_t port_;
    const char key_[16] = "SFvHsNFyyvOC2I0";
    static char key_received_[32];
    static std::atomic<bool> running_cond_;
};

char Client::key_received_[32] = "";
std::atomic<bool> Client::running_cond_{true};

/****************************************************************************************
 * UdpTest
 ****************************************************************************************/
class UdpTest : public ::testing::Test
{
public:
    UdpTest()
        : server_(port_),
          client_(ip_, port_)
    {}
    ~UdpTest() = default;

    Server& server() { return server_; }
    Client& client() { return client_; }

private:
    Server server_;
    Client client_;

    static const uint8_t ip_[4];
    static const uint16_t port_;
};

const uint8_t UdpTest::ip_[4] = {127, 0, 0, 1};
const uint16_t UdpTest::port_ = 2019;

/****************************************************************************************
 * Tests
 ****************************************************************************************/
TEST_F(UdpTest, Connection)
{
    ASSERT_EQ(server().init(), true);
    ASSERT_EQ(client().connect(), true);
}

TEST_F(UdpTest, Communication)
{
    ASSERT_EQ(server().init(), true);
    ASSERT_EQ(client().connect(), true);

    std::thread server_thread = std::thread(&Server::run, &server());
    std::thread client_thread = std::thread(&Client::run, &client());

    client_thread.join();
    server().stop();
    server_thread.join();

    const char* server_key = server().get_key();
    const char* client_key = client().get_key();
    ASSERT_EQ(memcmp(server_key, client_key, strlen(server_key)), 0);
}
