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

#pragma once

#include <unordered_map>
#include <memory>
#include <string>

#include <gio/gio.h>

/**
 * This class holds information about a D-Bus client connected to the server
 *
 * Each instance of this class is associated with an individual connection to a client, storing
 * information relevant to that client only.
 *
 * @attention CDBusClient instances and method calls shall occur from the same thread to avoid
 *            thread safety issue
 */
class CDBusClient
{
public:
    /** A unique and fixed id of the server a client is connected to */
    static const std::string SERVER_ID;

public:
    /**
     * Constructs an instance of this class
     *
     * With the construction of this instance, a new id will be automatically generated
     *
     * @param [in] connection  The D-Bus connection to associate with this instance
     *
     * @note The connection instance is guaranteed to don't get released during the
     *       lifetime of this client instance
     */
    CDBusClient(GDBusConnection *connection);

    /**
     * Destructor
     *
     * @note The connection instance will only be released if no other reference to it exists.
     *       Otherwise, only the reference counter is decremented.
     */
    ~CDBusClient();

    /**
     * Provides the D-Bus connection instance associated with this client
     *
     * @return The D-Bus connection instance
     */
    GDBusConnection *connection() const { return m_connection; };

    /**
     * Provides the D-Bus connection id associated with the connection to this client
     *
     * @return The D-Bus connection id
     */
    const std::string &id() const { return m_id; };

    /**
     * Provides the name associated with D-Bus connection of this client
     *
     * @return The name of the D-Bus connection
     */
    const std::string &name() const { return m_name; };

    /**
     * Set the name associated with D-Bus connection of this client
     *
     * @param [in] name The name associated with the connection of this client
     *
     * @note Setting the name does not affect the connection itself. This is just a placeholder
     *       to keep track of the name. The caller is responsible for managing name associations
     *       with connections / clients.
     */
    void name(const std::string &name) { m_name = name; };

private:
    /** The D-Bus connection id of this client */
    std::string m_id;
    /** The D-Bus connection name of this client */
    std::string m_name;
    /** The D-Bus connection instance */
    GDBusConnection *m_connection;
};

/**
 * This class provides a registry-like functionality to track the D-Bus connections / clients
 * connected to the server.
 *
 * @attention CDBusClientRegistry instances and method calls shall occur from the same thread to
 *            avoid thread safety issue
 */
class CDBusClientRegistry
{
public:
    /**
     * Add a new connection to be tracked
     *
     * A caller may use this method when a new connection is established with a client to keep
     * track of the client and the connection instance alive
     *
     * @param [in] connection The D-Bus connection established with a client
     *
     * @return A new client instance associated with the provided connection
     */
    static CDBusClient *add(GDBusConnection *connection);

    /**
     * Removes a connection from being tracked
     *
     * A caller may use this method when a connection is closed by a client to allow releasing
     * associated resources
     *
     * @param [in] connection The D-Bus connection previously established with a client
     */
    static void remove(GDBusConnection *connection);

    /**
     * Provides the client instance associated with the provided connection
     *
     * The connection must have been previously added to the registry via the call to
     * CDBusClientRegistry::add
     *
     * @param [in] connection The D-Bus connection established with a client
     *
     * @return A client instance associated with the provided connection, or nullptr if none is
     *         tracked
     */
    static CDBusClient *getByConnection(GDBusConnection *connection);

    /**
     * Provides the client instance associated with the provided connection id
     *
     * The connection with the corresponding id must have been previously added to the registry
     * via the call to CDBusClientRegistry::add
     *
     * @param [in] id The D-Bus connection id for the connection established with a client
     *
     * @return A client instance associated with the provided id, or nullptr if none is tracked
     */
    static CDBusClient *getById(const std::string &id);

    /**
     * Provides the client instance associated with the provided connection name
     *
     * The connection with the corresponding name must have been previously added to the registry
     * via the call to CDBusClientRegistry::add
     *
     * @param [in] name The D-Bus connection name for the connection established with a client
     *
     * @return A client instance associated with the provided name, or nullptr if none is tracked
     */
    static CDBusClient *getByName(const std::string &name);

    /**
     * Invokes a provided handler function for each registered client
     *
     * This method can be used whenever a system state change requires an action for all connected
     * clients (e.g. send a notification)
     *
     * @param [in] handler A callback function invoked for each tracked client
     */
    static void execute(std::function<void(const CDBusClient &client)> handler);

public:
    /**
     * Map of all tracked clients
     *
     * @note We use the connection as key for simplicity. This is ok, because this map tracks
     *       only connections who's object still exists, so no colision is possible in case a
     *       new connection has the same pointer as a previously closed one
     */
    static std::unordered_map<GDBusConnection *, CDBusClient *> clients;
};
