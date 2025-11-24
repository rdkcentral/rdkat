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

#include <gio/gio.h>

class CAtspiRegistryService;

/**
 * Helper class to store information that needs to be passed in async calls
 */
class CContext
{
public:
    /**
     * Frees an instance of this class
     */
    static void destroyContext(gpointer user_data)
    {
        CContext *context = static_cast<CContext *>(user_data);

        delete context;
    }

public:
    /**
     * Constructs an instance of this class
     *
     * @param [in] instance   A registry service instance to associate with this context
     * @param [in] connection A connection instance to associate with this context
     * 
     * @note The connection instance is guaranteed to don't be released during the lifetime
     *       of this CContext instance
     */
    CContext(CAtspiRegistryService *instance, GDBusConnection *connection)
        : m_instance(instance), m_connection(connection)
    {
        g_object_ref(m_connection);
    }

    /**
     * Destructor
     * 
     * @note The connection instance will only be released if no other references to it exist.
     *       Otherwise, only the reference counter is decremented.
     */
    virtual ~CContext()
    {
        g_object_unref(m_connection);
    }

    /**
     * Provides the registry service instance
     * 
     * @return The registry service instance associated with this context
     */
    CAtspiRegistryService *instance() const { return m_instance; }

    /**
     * Provides the connection instance
     * 
     * @return The connection instance associated with this context
     */
    GDBusConnection *connection() { return m_connection; }

private:

    /** The registry service instance */
    CAtspiRegistryService *m_instance;

    /** The connection instance */
    GDBusConnection *m_connection;
};