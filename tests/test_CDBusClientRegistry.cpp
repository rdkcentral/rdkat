#include <gio/gio.h>
#include <glib.h>
#include <thread>

#include "gtest/gtest.h"

#include "CDBusClientRegistry.h"

namespace CDBusClientRegistryTests
{

class CDBusClientRegistryTest : public ::testing::Test
{
public:
    static const std::size_t MAX_CONNECTIONS = 3;

protected:
    CDBusClientRegistryTest() = default;
    virtual ~CDBusClientRegistryTest() = default;

    virtual void SetUp()
    {
        for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
        {
            // Hard cast GObject to DBus connection for test purpose only, as altough CDBusClientRegistry
            // interface receives a GDBusConnection, it cares for it only as an GObject. This avoids
            // added complexity of mocking GDBusConnection
            m_connections.push_back(static_cast<GDBusConnection *>(g_object_new(G_TYPE_OBJECT, NULL)));
        }
    }

    virtual void TearDown()
    {
        for (auto it = m_connections.begin(); it != m_connections.end(); it++)
        {
            g_object_unref(*it);
        }
        m_connections.clear();
    }

public:
    GDBusConnection *connection(std::size_t index)
    {
        return (index < m_connections.size()) ? m_connections.at(index) : nullptr;
    }

protected:
    std::vector<GDBusConnection *> m_connections;
};

TEST_F(CDBusClientRegistryTest, tracks_client_connections)
{
    std::vector<CDBusClient *> clients;

    // Track N connections
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        CDBusClient *client = CDBusClientRegistry::add(connection(i));

        ASSERT_NE(nullptr, client);

        clients.push_back(client);
    }

    // Verify CDBusClient instances track correct connection
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ASSERT_EQ(connection(i), clients.at(i)->connection());
    }

    // Verify CDBusClient instances are retrievable by connection
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ASSERT_EQ(clients.at(i), CDBusClientRegistry::getByConnection(connection(i)));
    }

    // Verify connections are correctly removed without affecting others
    for (std::size_t r = 0; r < MAX_CONNECTIONS; r++)
    {
        CDBusClientRegistry::remove(connection(r));

        // Assign nullptr to allow same loop below to validate connections that still
        // exist and that the ones removed are not tracked anymore
        clients.at(r) = nullptr;

        for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
        {
            ASSERT_EQ(clients.at(i), CDBusClientRegistry::getByConnection(connection(i)));
        }
    }
}

TEST_F(CDBusClientRegistryTest, get_client_by)
{
    std::vector<CDBusClient *> clients;
    const int test_target = 1;

    // Track N connections
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        CDBusClient *client = CDBusClientRegistry::add(connection(i));

        ASSERT_NE(nullptr, client);

        client->name("Name_" + std::to_string(i));
        clients.push_back(client);
    }

    CDBusClient *client = clients.at(test_target);

    ASSERT_EQ(client, CDBusClientRegistry::getByConnection(connection(test_target)));
    ASSERT_EQ(client, CDBusClientRegistry::getById(client->id()));
    ASSERT_EQ(client, CDBusClientRegistry::getByName(client->name()));

    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        CDBusClientRegistry::remove(connection(i));
    }
}

TEST_F(CDBusClientRegistryTest, execute)
{
    std::vector<CDBusClient *> clients;

    // Track N connections
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        CDBusClient *client = CDBusClientRegistry::add(connection(i));

        ASSERT_NE(nullptr, client);
        clients.push_back(client);
    }

    CDBusClientRegistry::execute([&clients](const CDBusClient &client)
                                 {
                                    // Remove each client from our local copy so we can validate
                                    // that we got a call for all
                                    for(auto it = clients.begin(); it != clients.end(); it++)
                                    {
                                        if ((*it)->connection() == client.connection()) 
                                        {
                                            clients.erase(it);
                                            break;
                                        }
                                    } });
    CDBusClientRegistry::clear([](const CDBusClient &client) {});

    ASSERT_EQ(0, clients.size());
}

TEST_F(CDBusClientRegistryTest, clear_tracked_client_connections)
{
    std::vector<CDBusClient *> clients;

    // Track N connections
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        CDBusClient *client = CDBusClientRegistry::add(connection(i));

        ASSERT_NE(nullptr, client);
        clients.push_back(client);
    }

    CDBusClientRegistry::clear([&clients](const CDBusClient &client)
                               {
                                    // Remove each client from our local copy so we can validate
                                    // that we got a call for all
                                    for(auto it = clients.begin(); it != clients.end(); it++)
                                    {
                                        if ((*it)->connection() == client.connection()) 
                                        {
                                            clients.erase(it);
                                            break;
                                        }
                                    } });
    ASSERT_EQ(0, clients.size());

    // Verify no more client connections are tracked (the callback may have been called,
    // but the entry not removed)
    for (std::size_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ASSERT_EQ(nullptr, CDBusClientRegistry::getByConnection(connection(i)));
    }    
}

} // namespace CDBusClientRegistryTests