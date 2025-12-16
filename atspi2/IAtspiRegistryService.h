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

#include <cstdint>
#include <string>
#include <functional>

#include "IAtspiAccessibleObject.h"

/**
 * Interface for AT-SPI2 based registry service
 *
 * An implementation of this interface shall provide the required functionality to allow
 * accessibility related data providers (e.g. applications) to connect to the service and
 * interact with it following the AT-SPI2 protocol (uses D-Bus).
 * The implementation shall also allow data consumers / Assistive Technologies (AT) (e.g.
 * Screen Readers) to get notified of relevant events and interact with the data provider
 * using the provided abstractions.
 *
 * The service can be started using IAtspiRegistryService::start and stopped using
 * IAtspiRegistryService::stop methods. Once started, the service shall be ready to accept
 * client connections and interact with them per AT-SPI2 protocol. Once stopped, the service
 * will terminate all interactions and won't accept any new connections.
 *
 * @attention It is expected that the caller runs a glib loop which will be the one used to run
 *            the D-Bus server on. A dedicated thread may be used for this purpose.
 *            Public methods shall be called always from the same thread context to avoid thread
 *            safety issues or need to implement additional locking mechanisms to ensure thread
 *            safety.
 */
class IAtspiRegistryService
{
public:
    /**
     * Signature of an event listener callback function
     *
     * This function provides data related with the event and an accessible object interface that
     * allows querying more information from the originator of the event.
     *
     * @param [in] event  The name of the event. E.g. "Object:StateChanged"
     * @param [in] detail Event specific description. E.g. "focused"
     * @param [in] data1  Event specific extra information. E.g "1" or "0" in case of a
     *                    "Object:StateChange" event with the "focused" detail will indicate
     *                    whether the object is focused or not, respectively
     * @param [in] data2  Event specific extra information.
     * @param [in] object An accessible object instance associated with the event.
     *
     * @note The object remains valid only during the lifetime of the function call and shall not
     *       be stored for later use after this function returns
     */
    using EventListener = std::function<void(const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object)>;

public:
    /**
     * Start the AT-SPI2 registry service
     *
     * Starting the service implies the service will be ready to accept new connections from
     * clients once this function returns. Clients will be able to interact with the registry
     * using the AT-SPI2 protocol
     *
     * @attention This method captures the thread default context which will be the one the D-Bus
     *            server shall be running on. The caller needs to ensure the corresponding loop is
     *            run on the same context.
     *
     * @see IAtspiRegistryService::stop
     */
    virtual void start() = 0;

    /**
     * Stops the AT-SPI2 registry service
     *
     * Stopping the service implies the service stop accepting new connections from clients
     * once this function returns. Clients will not be able to interact anymore with the
     * registry using the AT-SPI2 protocol
     *
     * @attention This method shall be invoked from the same thread context
     *            IAtspiRegistryService::start was called from
     *
     * @see IAtspiRegistryService::start
     */
    virtual void stop() = 0;

    /**
     * Returns the address on which the service is running
     *
     * @return The address on which the registry service is running
     */
    virtual std::string address() = 0;

    /**
     * Register a callback function to be invoked when the specified event is received
     *
     * This method can be called before or after starting the service
     *
     * @param [in] event    The name of the event we are interested in. E.g.
     *                      "Document:LoadComplete", "Object:StateChanged"
     * @param [in] listener The callback function to be invoked when the event is received
     *
     * @attention This method shall be invoked from the same thread context
     *            IAtspiRegistryService::start was called from. The listener will be called
     *            from the same thread context as well.
     *
     * @see IAtspiRegistryService::unregisterEventListener
     */
    virtual void registerEventListener(const std::string &event, EventListener listener) = 0;

    /**
     * Unregister a previously registered callback function for the specified event
     *
     * This method can be called before or after stopping the service
     *
     * @param [in] event    The name of the event for which the listener shall be removed. E.g.
     *                      "Document:LoadComplete", "Object:StateChanged"
     *
     * @attention This method shall be invoked from the same thread context
     *            IAtspiRegistryService::start was called from
     *
     * @see IAtspiRegistryService::registerEventListener
     */
    virtual void unregisterEventListener(const std::string &event) = 0;

    /**
     * Destructor
     */
    virtual ~IAtspiRegistryService() {};
};
