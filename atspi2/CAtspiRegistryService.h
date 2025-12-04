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
#include <functional>
#include <unordered_map>
#include <mutex>

#include <gio/gio.h>
#include <glib.h>

#include "IAtspiRegistryService.h"
#include "IAtspiAccessibleObject.h"

/**
 * Implementation of the IAtspiRegistryService interface
 *
 * @note This is not a full implementation of the AT-SPI2 registry service capabilities, but
 *       rather a trimed down version to allow interactions with clients using the AT-SPI2
 *       protocol for the purpose of providing Text-To-Speech capabilities to an application
 *
 * @see Check IAtspiRegistryService for constraints and assumptions related with thread safety
 */
class CAtspiRegistryService : public IAtspiRegistryService
{
public:
    /**
     * Constructs an instance of the registry service
     *
     * @param [in] address The socket address to run the D-Bus server on
     */
    CAtspiRegistryService(const std::string &address);

    virtual ~CAtspiRegistryService();

    virtual void start();
    virtual void stop();

    virtual std::string address();

    virtual void registerEventListener(const std::string &event, EventListener listener);
    virtual void unregisterEventListener(const std::string &event);

private:
    /** A registration ID type used whenever a registration needs some tracking id */
    using RegistrationID = uint32_t;

    /**
     * Signature of the D-Bus method callback function
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     */
    using DBusMethodCallHandler = void (CAtspiRegistryService::*)(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /** Map that links method names with their handlers */
    using DBusMethodCallHandlerMap = std::unordered_map<std::string, DBusMethodCallHandler>;

    /** Map that links D-Bus connection instances with registration IDs (e.g. event listeners) */
    using DBusConnectionRegistrationIDsMap = std::unordered_map<GDBusConnection *, RegistrationID>;

    /**
     * Helper structure to track subscribed signals' registration ID for each connection
     * for a given event
     */
    typedef struct _EventListenerInfo
    {
        /**
         * Map of registration IDs associated with connections
         *
         * @note Use connection as key for simplicity. This is ok, because this map tracks only
         *       connections who's object still exists, so no colision is possible in case a new
         *       connection has the same pointer as a previously closed one
         */
        DBusConnectionRegistrationIDsMap registration_ids;

        /** The associated event callback function */
        EventListener listener;
    } EventListenerInfo;

    /** Defines an invalid registration ID */
    static inline constexpr RegistrationID REGISTRATION_ID_INVALID = 0;

    /**
     * Defines the D-Bus name request reply indicating the caller is the primary owner of the name
     */
    static inline constexpr guint DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER = 1;

    /**
     * Defines the D-Bus name request reply indicating the caller is waiting in a queue to become
     * the name owner
     */
    static inline constexpr guint DBUS_REQUEST_NAME_REPLY_IN_QUEUE = 2;

    /**
     * Defines the D-Bus name request reply indicating the name already has an owner and
     * configuration flags won't allow the caller to become one
     */
    static inline constexpr guint DBUS_REQUEST_NAME_REPLY_EXISTS = 3;

    /**
     * Defines the D-Bus name request reply indicating the caller is already the owner
     */
    static inline constexpr guint DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER = 4;

private:
    /**
     * Register the relevant D-Bus interfaces with a D-Bus connection, making them available
     * for interactions with clients
     *
     * @param [in] connection The D-Bus connection on which the interfaces shall be registered on
     *
     * @see CAtspiRegistryService::unregisterDBusInterfaces
     */
    void registerDBusInterfaces(GDBusConnection *connection);

    /**
     * Unregister the relevant D-Bus interfaces from the D-Bus connection, making them unavailable
     * for interactions with clients
     *
     * @param [in] connection The D-Bus connection on which the interfaces shall be unregistered
     *                        from
     *
     * @ see CAtspiRegistryService::registerDBusInterfaces
     */
    void unregisterDBusInterfaces(GDBusConnection *connection);

    /**
     * Register the relevant AT-SPI2 registry service interfaces with a D-Bus connection, making
     * them available for interactions with clients
     *
     * @param [in] connection The D-Bus connection on which the interfaces shall be registered on
     *
     * @see CAtspiRegistryService::unregisterRegistryInterfaces
     */
    void registerRegistryInterfaces(GDBusConnection *connection);

    /**
     * Unregister the relevant AT-SPI2 registry service interfaces from the D-Bus connection,
     * making them unavailable for interactions with clients
     *
     * @param [in] connection The D-Bus connection on which the interfaces shall be unregistered
     *                        from
     *
     * @see CAtspiRegistryService::registerRegistryInterfaces
     */
    void unregisterRegistryInterfaces(GDBusConnection *connection);

    /**
     * Register a D-Bus compliant interface with a D-Bus connection and specific object path,
     * making it available for interactions with clients
     *
     * @param [in] connection    The D-Bus connection on which the interface shall be registered on
     * @param [in] object_path   The object path the interface shall be made available on
     * @param [in] interface_xml A D-Bus interface XML specification
     * @param [in] vtable        Table with callback functions specific for the provided interface
     *
     * @see CAtspiRegistryService::unregisterInterface
     */
    RegistrationID registerInterface(GDBusConnection *connection,
                                     const std::string &object_path,
                                     const std::string &interface_xml,
                                     const GDBusInterfaceVTable *vtable);

    /**
     * Unegister a D-Bus compliant interface from the D-Bus connection, making it unavailable for
     * interactions with clients
     *
     * @param [in] connection      The D-Bus connection from which the interface shall be
     *                             unregistered from
     * @param [in] registration_id The registration ID of the interface to unregister
     *
     * @see CAtspiRegistryService::registerInterface
     */
    void unregisterInterface(GDBusConnection *connection, RegistrationID registration_id);

    /**
     * Send a signal over D-Bus on the provided connection
     *
     * @param [in] connection  The D-Bus connection on which the signal shall be send
     * @param [in] object_path The object path that shall be associated with the signal
     * @param [in] interface   The interface that describes the signal
     * @param [in] signal      The signal (name) to send
     * @param [in] parameter   The parameters to send with the signal. If parameters is floating,
     *                         we assume ownership of parameters
     *
     * @attention Signal will be emitted from the D-Bus thread context
     */
    void emitSignal(GDBusConnection *connection, const std::string &object_path, const std::string &interface, const std::string &signal, GVariant *parameter);

    /**
     * Start listening to D-Bus signals that are required and that we are not yet listening for
     *
     * A user of this service can register listeners for events via
     * CAtspiRegistryService::registerEventListener which may require listening for signals. If a
     * connection was not established yet, we cannot subscribe to the required signals yet and
     * need to subscribe only when the connection is established. If a connection is already
     * established, we can subsribe right away. This method will take care of both scenarios.
     *
     * @attention This method shall be called from the D-Bus thread context
     */
    void subscribeSignalsPending();

    /**
     * Stop listening to D-Bus signals that the service is currently listening for
     *
     * @attention This method shall be called from the D-Bus thread context
     */
    void unsubscribeSignalsActive(GDBusConnection *connection);

    /**
     * Start listen for a specific signal over D-Bus on the provided connection
     *
     * @param [in] connection The D-Bus connection on which we shall listen for the signal
     * @param [in] interface  The interface that describes the signal
     * @param [in] signal     The signal (name) to listen for
     * @param [in] callback   The function to invoke when the given signal is received
     *
     * @return A registration ID associated with the signal we subscribed to. Use it to unsubscribe
     *         to the signal
     *
     * @see CAtspiRegistryService::unsubscribeSignal
     */
    RegistrationID subscribeSignal(GDBusConnection *connection, const std::string &interface, const std::string &signal, GDBusSignalCallback callback);

    /**
     * Stop listen for a signal over D-Bus on the provided connection
     *
     * @param [in] connection The D-Bus connection associated with the signal we are listening for
     * @param [in] id         The registration ID associated with the signal we started previously
     *                        to listen for
     *
     * @see CAtspiRegistryService::subscribeSignal
     */
    void unsubscribeSignal(GDBusConnection *connection, RegistrationID id);

    /**
     * Callback handler for a D-Bus "Hello" method call send by a client
     *
     * This will provide a unique name for the client and is required before any further interactions
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return Unique name via the @p invocation instance
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "Hello" method call
     */
    void handleDBusHello(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a D-Bus "StartServiceByName" method call send by a client
     *
     * This method is used to activate a service (e.g. Registry service)
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return Service start result via the @p invocation instance
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "StartServiceByName" method call
     */
    void handleDBusStartServiceByName(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a D-Bus "GetNameOwner" method call send by a client
     *
     * Returns the unique connection name of the primary owner of the name given
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return The name owner via the @p invocation instance for the provided name
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "GetNameOwner" method call
     */
    void handleDBusGetNameOwner(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a D-Bus "AddMatch" method call send by a client
     *
     * Adds a match rule to match messages going through the message bus
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return Nothing
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "AddMatch" method call
     */
    void handleDBusAddMatch(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a D-Bus "RemoveMatch" method call send by a client
     *
     * Removes a match rule to match messages going through the message bus
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return Nothing
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "RemoveMatch" method call
     */
    void handleDBusRemoveMatch(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a D-Bus "RequestName" method call send by a client
     *
     * Requests to have the given name assigned to the method caller
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return Status of the name assignment
     *
     * @see D-Bus "org.freedesktop.DBus" interface specification for "RequestName" method call
     */
    void handleDBusRequestName(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Callback handler for a AT-SPI protocol "GetRegisteredEvents" method call send by a client
     *
     * Provides the list of events a Assistive Technology (e.g. Screen Reader) is listening for.
     * This can be used by the caller to decide wether to send those events or not, depending if
     * anyone is listening for them.
     *
     * @param [in] connection The D-Bus connection this callback is invoked for
     * @param [in] parameters Parameters provided with the method call
     * @param [in] invocation A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     *
     * @return List of events being listened for
     *
     * @see AT-SPI2 "org.a11y.atspi.Registry" interface specification for "GetRegisteredEvents"
     *      method call
     */
    void handleAtspi2GetRegisteredEvents(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation);

    /**
     * Provides the registration ID for the registered D-Bus interfaces
     *
     * @param [in] connection The D-Bus connection where the interfaces were registered with
     *
     * @return The registration ID associated with the registered interfaces or
     *         REGISTRATION_ID_INVALID if a registration cannot be found
     */
    RegistrationID registrationIDForDBusInterfaces(GDBusConnection *connection) const;

    /**
     * Provides the registration ID for the registered AT-SPI2 Registry interfaces
     *
     * @param [in] connection The D-Bus connection where the interfaces were registered with
     *
     * @return The registration ID associated with the registered interfaces or
     *         REGISTRATION_ID_INVALID if a registration cannot be found
     */
    RegistrationID registrationIDForAtspi2RegistryInterfaces(GDBusConnection *connection) const;

    /**
     * Saves the registration ID associated with the registered D-Bus interfaces
     *
     * @param [in] connection      The D-Bus connection where the interfaces were registered with
     * @param [in] registration_id The registration ID associated with the registered interfaces
     */
    void registrationIDForDBusInterfaces(GDBusConnection *connection, RegistrationID registration_id);

    /**
     * Saves the registration ID associated with the registered AT-SPI2 Registry interfaces
     *
     * @param [in] connection The D-Bus connection where the interfaces were registered with
     * @param [in] registration_id The registration ID associated with the registered interfaces
     */
    void registrationIDForAtspi2RegistryInterfaces(GDBusConnection *connection, RegistrationID registration_id);

private:
    /**
     * Joins the short event identifier string like "Document:Loadcomplete" with the full interface
     * to create a string like "org.a11y.atspi.Event.Document:Loadcomplete"
     *
     * @param [in] event The short event identifier that excludes the interface name
     *
     * @return A string with the event interface name joined with the event identifier
     */
    static std::string addEventInterface(const std::string &event);

    /**
     * Removes an event interface name from a string like "org.a11y.atspi.Event.Document:Loadcomplete"
     * to have only the short event identifier like "Document:Loadcomplete"
     *
     * @param [in] event The full event identifier that includes the interface name
     *
     * @return A string with the short event identifier
     */
    static std::string removeEventInterface(const std::string &event);

    /**
     * Split the signal portion from a full event identifier into the interface and signal parts.
     *
     * This splits something like "Document:Loadcomplete" into a pair<Document, LoadComplete>, or
     * something like "org.a11y.atspi.Event.Document:LoadComplete" into
     * pair<org.a11y.atspi.Event.Document, LoadComplete>
     *
     * @param [in] event The event identifier that may include or exclude the interface name
     *
     * @return A pair containing the interface name and signal
     */
    static std::pair<std::string, std::string> splitInterfaceAndSignal(const std::string &event);

    /**
     * Joins an interface and signal names together to build an event name.
     *
     * This will create from an interface and signal like "org.a11y.atspi.Event.Document" and
     * "Loadcomplete", a single event identifier like "org.a11y.atspi.Event.Document:Loadcomplete".
     * This also applies to the short event version to create from "Document" and "Loadcomplete",
     * a single identifier like "Document:Loadcomplete"
     *
     * @param [in] interface The interface name to join
     * @param [in] signal    The signal name to join
     *
     * @return A string with the event identifier
     */
    static std::string joinInterfaceAndSignal(const std::string &interface, const std::string &signal);

    /**
     * Callback wrapper from C to C++ to handle a new connection being established from a client
     *
     * @param [in] server     The D-Bus server instance with which the new connection was established
     * @param [in] connection The new D-Bus connection instance
     * @param [in] user_data  Custom data provided when the wrapper callback was registered
     *
     * @return Boolean indicating whether the connection shall be claimed or not
     *
     * @retval true  To claim the connection
     * @retval false To let other handlers run
     */
    static gboolean onNewConnectionWrapper(GDBusServer *server, GDBusConnection *connection, gpointer user_data);

    /**
     * Callback wrapper from C to C++ to handle an existing connection being closed
     *
     * @param [in] connection            The D-Bus connection instance being closed
     * @param [in] remote_peer_vanished  If true, indicates that the remote peer closed the connection
     * @param [in] error                 An error with more details about the event or NULL if no error
     * @param [in] user_data             Custom data provided when the wrapper callback was registered
     */
    static void onClosedConnectionWrapper(GDBusConnection *connection, gboolean remote_peer_vanished, GError *error, gpointer user_data);

    /**
     * Callback wrapper from C to C++ to handle a D-Bus interface specific method call
     *
     * @param [in] connection     The new D-Bus connection instance
     * @param [in] sender         The D-Bus sender id that made this method call
     * @param [in] object_path    The object path the interface shall be made available on
     * @param [in] interface_name The interface name associated associated with the method call
     * @param [in] method_name    The method being called
     * @param [in] parameters     Arguments provided to the method call
     * @param [in] invocation     A D-Bus connection invocation instance to allow return of results
     *                        or errors in an async way
     * @param [in] user_data      Custom data provided when the wrapper callback was registered
     */
    static void onDBusMethodCallWrapper(GDBusConnection *connection,
                                        const gchar *sender,
                                        const gchar *object_path,
                                        const gchar *interface_name,
                                        const gchar *method_name,
                                        GVariant *parameters,
                                        GDBusMethodInvocation *invocation,
                                        gpointer user_data);

    /**
     * Callback wrapper from C to C++ to handle a AT-SPI2 registry interface specific method call
     *
     * @param [in] connection     The new D-Bus connection instance
     * @param [in] sender         The D-Bus sender id that made this method call
     * @param [in] object_path    The object path associated with the method call
     * @param [in] interface_name The interface name associated associated with the method call
     * @param [in] method_name    The method being called
     * @param [in] parameters     Arguments provided to the method call
     * @param [in] invocation     A D-Bus connection invocation instance to allow return of results
     *                            or errors in an async way
     * @param [in] user_data      Custom data provided when the wrapper callback was registered
     */
    static void onAtspi2RegistryMethodCallWrapper(GDBusConnection *connection,
                                                  const gchar *sender,
                                                  const gchar *object_path,
                                                  const gchar *interface_name,
                                                  const gchar *method_name,
                                                  GVariant *parameters,
                                                  GDBusMethodInvocation *invocation,
                                                  gpointer user_data);

    /**
     * Callback wrapper from C to C++ to handle a D-Bus signal being received
     *
     * @param [in] connection     The new D-Bus connection instance
     * @param [in] sender         The D-Bus sender id that sent the signal
     * @param [in] object_path    The object path associated with the method call
     * @param [in] interface_name The interface name associated associated with the method call
     * @param [in] signal_name    The received signal name
     * @param [in] parameters     Arguments provided with the signal
     * @param [in] user_data      Custom data provided when the wrapper callback was registered
     */
    static void onSignalWrapper(GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *signal_name,
                                GVariant *parameters,
                                gpointer user_data);

private:
    /**
     * Handle a new connection being established from a client
     *
     * @param [in] connection The new D-Bus connection instance
     *
     * @return Boolean indicating whether the connection shall be claimed or not
     *
     * @retval true  To claim the connection
     * @retval false To let other handlers run
     */
    gboolean onNewConnection(GDBusConnection *connection);

    /**
     * Handle an existing connection being closed
     *
     * @param [in] connection      The D-Bus connection instance being closed
     * @param [in] update_registry Updates the client registry accordingly
     *
     * @note Called from the D-Bus context
     */
    void onClosedConnection(GDBusConnection *connection, bool update_registry = true);

    /**
     * Callback to process method calls send over D-Bus by a client
     *
     * @param [in] handlers    Map that associates method names with a method specific handler
     * @param [in] connection  The new D-Bus connection instance
     * @param [in] method_name The method being called
     * @param [in] parameters  Arguments provided to the method call
     * @param [in] invocation  A D-Bus connection invocation instance to allow return of results
     *                         or errors in an async way
     */
    void onMethodCall(const DBusMethodCallHandlerMap &handlers,
                      GDBusConnection *connection,
                      const gchar *method_name,
                      GVariant *parameters,
                      GDBusMethodInvocation *invocation);

    /**
     * Callback to process signals send over D-Bus by a client
     *
     * @param [in] connection  The D-Bus connection instance on which the signal was received
     * @param [in] object_path The object path associated with the signal
     * @param [in] event       The name of the event received. E.g. "Object:StateChanged"
     * @param [in] detail      Event specific description. E.g. "focused"
     * @param [in] data1       Event specific extra information. E.g "1" or "0" in case of a
     *                         "Object:StateChange" event with the "focused" detail will indicate
     *                         whether the oject is focused or not, respectivly
     * @param [in] data2       Event specific extra information.
     */
    void onSignal(GDBusConnection *connection,
                  const std::string object_path,
                  const std::string event,
                  const std::string detail,
                  uint32_t data1,
                  uint32_t data2);

    /**
     * Indicates whether the service has been started and is active or not
     *
     * @return Boolean indicating whether the service is active
     *
     * @retval true  Service is active (was started)
     * @retval false Service is not active (was stopped or not started at all)
     */
    bool active() const;

private:
    /** The D-Bus server instance */
    GDBusServer *m_dbus_server = nullptr;

    /** The GLib main context used */
    GMainContext *m_dbus_context = nullptr;

    /** The D-Bus server socket address */
    std::string m_address;

    /** Registration IDs for the D-Bus interfaces registration per connection */
    std::unordered_map<GDBusConnection *, RegistrationID> m_dbus_registration_id;

    /** Registration IDs for the ATSPI2 registry interfaces registration per connection */
    std::unordered_map<GDBusConnection *, RegistrationID> m_registry_registration_id;

    /** Method handlers for the D-Bus interfaces */
    const std::unordered_map<std::string, DBusMethodCallHandler> m_dbus_method_handlers = {
        {"Hello", &CAtspiRegistryService::handleDBusHello},
        {"RequestName", &CAtspiRegistryService::handleDBusRequestName},
        {"StartServiceByName", &CAtspiRegistryService::handleDBusStartServiceByName},
        {"GetNameOwner", &CAtspiRegistryService::handleDBusGetNameOwner},
        {"AddMatch", &CAtspiRegistryService::handleDBusAddMatch},
        {"RemoveMatch", &CAtspiRegistryService::handleDBusRemoveMatch}};

    /** Method handlers for the AT-SPI2 registry interfaces */
    const std::unordered_map<std::string, DBusMethodCallHandler> m_registry_method_handlers = {
        {"GetRegisteredEvents", &CAtspiRegistryService::handleAtspi2GetRegisteredEvents}};

    /** Registered event listeners */
    std::unordered_map<std::string, EventListenerInfo> m_event_listeners;
};
