/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <memory>

#include "CDBusClientRegistry.h"

std::unordered_map<GDBusConnection *, std::unique_ptr<CDBusClient>> CDBusClientRegistry::clients;
const std::string CDBusClient::SERVER_ID = ":1.0"; // Must not collide with id generated in CDBusClient()

CDBusClient::CDBusClient(GDBusConnection *connection)
{
    static uint32_t minor_id = 1; // Start at 1 to avoid collision with SERVER_ID

    g_assert(connection != nullptr);

    std::stringstream s;
    s << ":1." << minor_id++;
    m_id = s.str();

    #if !GLIB_CHECK_VERSION(2, 56, 0)
        #error "Glib version shall be 2.56 or more recent"
    #endif
    
    m_connection = g_object_ref(connection);
}

CDBusClient::~CDBusClient()
{
    g_object_unref(m_connection);
}

CDBusClient *CDBusClientRegistry::add(GDBusConnection *connection)
{
    auto client = new CDBusClient(connection);

    if (!client)
    {
        return nullptr;
    }

    auto result = clients.emplace(connection, client);

    return result.second ? client : nullptr;
}

void CDBusClientRegistry::remove(GDBusConnection *connection)
{
    clients.erase(connection);
}

CDBusClient *CDBusClientRegistry::getByConnection(GDBusConnection *connection)
{
    auto it = clients.find(connection);

    return (it != clients.end()) ? it->second.get() : nullptr;
}

CDBusClient *CDBusClientRegistry::getById(const std::string &id)
{
    auto it = std::find_if(clients.begin(), clients.end(), [id](const auto &entry)
                           { return entry.second->id() == id; });

    return (it != clients.end()) ? it->second.get() : nullptr;
}

CDBusClient *CDBusClientRegistry::getByName(const std::string &name)
{
    auto it = std::find_if(clients.begin(), clients.end(), [name](const auto &entry)
                           { return entry.second->name() == name; });

    return (it != clients.end()) ? it->second.get() : nullptr;
}

void CDBusClientRegistry::execute(std::function<void(const CDBusClient &client)> handler)
{
    for (auto const &entry : clients)
    {
        std::invoke(handler, *(entry.second));
    }
}

void CDBusClientRegistry::clear(std::function<void(const CDBusClient &client)> handler)
{
    for (auto it = clients.begin(); it != clients.end(); )
    {
        std::invoke(handler, *(it->second));
        it = clients.erase(it);
    }
}
