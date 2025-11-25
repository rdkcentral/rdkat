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

#include <string>
#include <glib.h>

#include "CContext.h"

/**
 * Helper class to store information that needs to be passed in async signal emission calls
 */
class CContextSignalEmitter : public CContext
{
public:
    /**
     * Constructs an instance of this class
     *
     * @param [in] instance    A registry service instance to associate with this context
     * @param [in] connection  A connection instance to associate with this context
     * @param [in] object_path An accessible object's path to associate with this context
     * @param [in] interface   An interface identifier to associate with this context
     * @param [in] signal      An interface's signal to associate with this context
     * @param [in] parameter   A parameter variant to associate with this context
     * 
     * @note The connection and parameter instances are guaranteed to don't get released during the
     *       lifetime of this CContextSignalEmitter instance
     */
    CContextSignalEmitter(CAtspiRegistryService *instance,
                          GDBusConnection *connection,
                          std::string object_path,
                          std::string interface,
                          std::string signal,
                          GVariant *parameter)
        : CContext(instance, connection), m_object_path(object_path), m_interface(interface), m_signal(signal), m_parameter(parameter)
    {
        g_variant_ref(m_parameter);
    }

    /**
     * Destructor
     * 
     * @note The connection and parameter instances will only be released if no other references
     *       to it exist. Otherwise, only the reference counter is decremented.
     */
    virtual ~CContextSignalEmitter()
    {
        g_variant_unref(m_parameter);
    }

    /**
     * Provides the accessible object's path associated with this context
     *
     * @return The accessible object's path
     */
    std::string &objectPath() { return m_object_path; }

    /**
     * Provides the interface identifier associated with this context
     *
     * @return The interface identifier
     */
    std::string &interface() { return m_interface; }


    /**
     * Provides the interface's signal associated with this context
     *
     * @return The interface's signal
     */
    std::string &signal() { return m_signal; }


    /**
     * Provides the parameter variant associated with this context
     *
     * @return The parameter variant
     */
    GVariant *parameter() { return m_parameter; }

private:
    /** The accessible object's path */
    std::string m_object_path;

    /** The interface identifier */
    std::string m_interface;

    /** The interface's signal */
    std::string m_signal;

    /** The parameter variant */
    GVariant *m_parameter;
};