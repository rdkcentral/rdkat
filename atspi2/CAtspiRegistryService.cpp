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

#include <string>
#include <thread>

#include "logger.h"
#include "CAtspiRegistryService.h"
#include "CAtspiAccessibleObject.h"
#include "CDBusClientRegistry.h"
#include "CContextSignalEmitter.h"

// #define DBG_ATSPI_BUS 1 // Enable logging of D-Bus messages seen by the server

static const gchar g_dbus_introspection_xml[] =
    "<node>"
    "  <interface name=\"org.freedesktop.DBus\">"
    "    <method name=\"RequestName\">"
    "      <arg direction=\"in\" type=\"s\"/>"
    "      <arg direction=\"in\" type=\"u\"/>"
    "      <arg direction=\"out\" type=\"u\"/>"
    "    </method>"
    "    <method name=\"StartServiceByName\">"
    "      <arg direction=\"in\" type=\"s\"/>"
    "      <arg direction=\"in\" type=\"u\"/>"
    "      <arg direction=\"out\" type=\"u\"/>"
    "    </method>"
    "    <method name=\"Hello\">"
    "      <arg direction=\"out\" type=\"s\"/>"
    "    </method>"
    "    <method name=\"AddMatch\">"
    "      <arg direction=\"in\" type=\"s\"/>"
    "    </method>"
    "    <method name=\"GetNameOwner\">"
    "      <arg direction=\"in\" type=\"s\"/>"
    "      <arg direction=\"out\" type=\"s\"/>"
    "    </method>"
    "    <signal name=\"NameLost\">"
    "      <arg type=\"s\"/>"
    "    </signal>"
    "    <signal name=\"NameAcquired\">"
    "      <arg type=\"s\"/>"
    "    </signal>"
    "  </interface>"
    "</node>";

static const gchar g_registry_introspection_xml[] =
    "<node>"
    "  <interface name=\"org.a11y.atspi.Registry\">"
    "    <method name=\"GetRegisteredEvents\">"
    "      <arg direction=\"out\" name=\"events\" type=\"a(ss)\"/>"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QSpiEventListenerArray\"/>"
    "    </method>"
    "    <signal name=\"EventListenerRegistered\">"
    "      <arg name=\"bus\" type=\"s\"/>"
    "      <arg name=\"event\" type=\"s\"/>"
    "    </signal>"
    "    <signal name=\"EventListenerDeregistered\">"
    "      <arg name=\"bus\" type=\"s\"/>"
    "      <arg name=\"event\" type=\"s\"/>"
    "    </signal>"
    "  </interface>"
    "</node>";

#ifdef DBG_ATSPI_BUS
static GDBusMessage *dbg_dbus_message_filter(GDBusConnection *connection,
                                             GDBusMessage *message,
                                             gboolean incoming,
                                             gpointer user_data)
{
    GVariant *body = g_dbus_message_get_body(message);
    gchar *body_content = body ? g_variant_print(body, FALSE) : NULL;

    const char *types[] = {"invalid", "method_call", "method_return", "error", "signal"};

    g_print("D-Bus message - %s%s %s %d(%d) sender: %s destination: %s %s %s.%s\n     %s\n",
            "client id ",
            incoming ? "->" : "<-",
            types[g_dbus_message_get_message_type(message)],
            g_dbus_message_get_serial(message),
            g_dbus_message_get_reply_serial(message),
            g_dbus_message_get_sender(message),
            g_dbus_message_get_destination(message),
            g_dbus_message_get_path(message),
            g_dbus_message_get_interface(message),
            g_dbus_message_get_member(message),
            body_content ? body_content : "(no body)");

    if (body_content)
    {
        g_free(body_content);
    }
    return message;
}
#endif

CAtspiRegistryService::CAtspiRegistryService(const std::string &address)
    : m_address(address)
{
}

CAtspiRegistryService::~CAtspiRegistryService()
{
}

void CAtspiRegistryService::start()
{
    GError *error = NULL;

    RDKLOG_TRACE("Start AT-SPI2 registry service");

    if (m_dbus_server != nullptr)
    {
        RDKLOG_WARNING("D-Bus server is already running");
        return;
    }
    // Remember D-Bus server thread context
    m_dbus_context = g_main_context_ref_thread_default();

    if (m_dbus_context == nullptr)
    {
        RDKLOG_ERROR("Attempting to run D-Bus server on main context. Please set a thread default context before starting the service");
        return;
    }

    m_dbus_server = g_dbus_server_new_sync(m_address.c_str(),
                                           G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS,
                                           g_dbus_generate_guid(),
                                           NULL,
                                           NULL,
                                           &error);

    if (error != nullptr)
    {
        RDKLOG_ERROR("Unable to create D-Bus server: %s", error->message);

        g_error_free(error);

        g_main_context_unref(m_dbus_context);
        m_dbus_context = nullptr;
        return;
    }

    g_dbus_server_start(m_dbus_server);

    g_signal_connect(m_dbus_server,
                     "new-connection",
                     G_CALLBACK(onNewConnectionWrapper),
                     this);

    RDKLOG_INFO("AT-SPI2 registry service started on address = %s", m_address.c_str());
}

void CAtspiRegistryService::stop()
{
    RDKLOG_TRACE("Stop AT-SPI2 registry service");

    if (m_dbus_server == nullptr)
    {
        RDKLOG_WARNING("D-Bus server is not running");
        return;
    }
    g_dbus_server_stop(m_dbus_server);

    g_object_unref(m_dbus_server);

    g_main_context_unref(m_dbus_context);

    m_dbus_server = nullptr;
    m_dbus_context = nullptr;

    RDKLOG_INFO("AT-SPI2 registry service stopped");
}

std::string CAtspiRegistryService::address()
{
    return m_address;
}

std::string CAtspiRegistryService::addEventInterface(const std::string &event)
{
    const std::string full_event_interface("org.a11y.atspi.Event.");
    return full_event_interface + event;
}

std::string CAtspiRegistryService::removeEventInterface(const std::string &event)
{
    const std::string full_event_interface("org.a11y.atspi.Event.");

    g_assert(event.find(full_event_interface) != std::string::npos);

    return event.substr(full_event_interface.length());
}

std::pair<std::string, std::string> CAtspiRegistryService::splitInterfaceAndSignal(const std::string &event)
{
    std::size_t delimiter_index = event.find(":");

    g_assert(delimiter_index != std::string::npos);

    return std::make_pair(event.substr(0, delimiter_index), event.substr(delimiter_index + 1));
}

std::string CAtspiRegistryService::joinInterfaceAndSignal(const std::string &interface, const std::string &signal)
{
    return interface + ":" + signal;
}

gboolean CAtspiRegistryService::onNewConnectionWrapper(GDBusServer *server, GDBusConnection *connection, gpointer user_data)
{
    CAtspiRegistryService *instance = static_cast<CAtspiRegistryService *>(user_data);

    return instance->onNewConnection(connection);
}

void CAtspiRegistryService::onClosedConnectionWrapper(GDBusConnection *connection, gboolean remote_peer_vanished, GError *error, gpointer user_data)
{
    CAtspiRegistryService *instance = static_cast<CAtspiRegistryService *>(user_data);

    // Silence compiler warning of unused variable
    (void)remote_peer_vanished;
    (void)error;

    CContext *context = new CContext(instance, connection);

    g_assert(context != nullptr);

    g_main_context_invoke_full(instance->m_dbus_context, G_PRIORITY_DEFAULT, [](gpointer user_data) -> gboolean
                               {
                                   CContext *context = static_cast<CContext *>(user_data);

                                   context->instance()->onClosedConnection(context->connection());

                                   return FALSE; // Single shot
                               },
                               context, CContext::destroyContext);
}

gboolean CAtspiRegistryService::onNewConnection(GDBusConnection *connection)
{
    RDKLOG_INFO("Client connected");

    g_dbus_connection_set_exit_on_close(connection, FALSE);
    g_signal_connect(connection, "closed", G_CALLBACK(onClosedConnectionWrapper), this);

    CDBusClientRegistry::add(connection);

#ifdef DBG_ATSPI_BUS
    g_dbus_connection_add_filter(connection, dbg_dbus_message_filter, NULL, NULL);
#endif

    registerDBusInterfaces(connection);
    registerRegistryInterfaces(connection);

    // Signals will be subscribed only if an event listener was registered
    subscribeSignalsPending();
    return TRUE;
}

void CAtspiRegistryService::onClosedConnection(GDBusConnection *connection)
{
    RDKLOG_INFO("Client disconnected");

    unsubscribeSignalsActive(connection);

    unregisterRegistryInterfaces(connection);
    unregisterDBusInterfaces(connection);

    CDBusClientRegistry::remove(connection);
}

CAtspiRegistryService::RegistrationID CAtspiRegistryService::registerInterface(GDBusConnection *connection,
                                                                               const std::string &object_path,
                                                                               const std::string &interface_xml,
                                                                               const GDBusInterfaceVTable *vtable)
{
    g_assert(connection != nullptr);

    RDKLOG_TRACE("Register D-Bus interface");

    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(interface_xml.c_str(), NULL);
    if (introspection_data == nullptr)
    {
        RDKLOG_ERROR("Unable to create introspection data from xml");
        return REGISTRATION_ID_INVALID;
    }

    auto registration_id = g_dbus_connection_register_object(connection,
                                                             object_path.c_str(),
                                                             introspection_data->interfaces[0],
                                                             vtable, /* Will be copied */
                                                             this,   /* user_data */
                                                             NULL,   /* user_data_free_func */
                                                             NULL);  /* GError** */

    g_dbus_node_info_unref(introspection_data);

    return (registration_id != 0) ? registration_id : REGISTRATION_ID_INVALID;
}

void CAtspiRegistryService::unregisterInterface(GDBusConnection *connection, uint registration_id)
{
    RDKLOG_TRACE("Unregister D-Bus interface");

    g_assert(registration_id != REGISTRATION_ID_INVALID);
    g_assert(connection != nullptr);

    g_dbus_connection_unregister_object(connection, registration_id);
}

void CAtspiRegistryService::emitSignal(GDBusConnection *connection, const std::string &object_path, const std::string &interface, const std::string &signal, GVariant *parameter)
{
    RDKLOG_VERBOSE("Emit signal '%s' for object = '%s' and interface = '%s'", signal.c_str(), object_path.c_str(), interface.c_str());

    CContextSignalEmitter *context = new CContextSignalEmitter(this, connection, object_path, interface, signal, g_variant_ref_sink(parameter));

    g_assert(context != nullptr);

    // Can release now as tracked as part of context, if needed
    g_variant_unref(parameter);

    g_main_context_invoke_full(m_dbus_context, G_PRIORITY_DEFAULT, [](gpointer user_data) -> gboolean
                               {
                                   CContextSignalEmitter *context = static_cast<CContextSignalEmitter *>(user_data);

                                   g_assert(context != nullptr);
                                   g_assert(context->connection() != nullptr);
                                   g_assert(context->instance() != nullptr);

                                   // FIXME: We can't use g_dbus_connection_emit_signal() to emit the signal because it will not set
                                   // the 'sender' field in the message header. GLib D-Bus proxy implementation checks that the sender
                                   // name of the sender of the signal matches the owner the proxy is for (see 'on_signal_received' in
                                   // gdbusproxy.c), not propagating signal if there is a mismatch.
                                   // While this is not fixed, we need to create the message manually
                                   GDBusMessage *message;
                                   GError *error = NULL;

                                   message = g_dbus_message_new_signal(context->objectPath().c_str(),
                                                                       context->interface().c_str(),
                                                                       context->signal().c_str());

                                   g_dbus_message_set_header(message,
                                                             G_DBUS_MESSAGE_HEADER_FIELD_DESTINATION,
                                                             g_variant_new_string(CDBusClientRegistry::getByConnection(context->connection())->id().c_str()));

                                   if (context->parameter() != NULL)
                                   {
                                       g_dbus_message_set_body(message, context->parameter());
                                   }
                                   g_dbus_message_set_sender(message, CDBusClient::SERVER_ID.c_str());

                                   g_dbus_connection_send_message(context->connection(), message, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, &error);
                                   g_object_unref(message);

                                   return FALSE; // Single shot
                               },
                               context, CContext::destroyContext);
}

CAtspiRegistryService::RegistrationID CAtspiRegistryService::subscribeSignal(GDBusConnection *connection, const std::string &interface, const std::string &signal, GDBusSignalCallback callback)
{
    RDKLOG_VERBOSE("Subscribe signal '%s' for interface = '%s'", signal.c_str(), interface.c_str());

    auto id = g_dbus_connection_signal_subscribe(connection,
                                                 NULL,
                                                 interface.c_str(),
                                                 signal.c_str(),
                                                 NULL,
                                                 NULL,
                                                 G_DBUS_SIGNAL_FLAGS_NONE,
                                                 callback,
                                                 this,
                                                 NULL);

    return static_cast<RegistrationID>(id);
}

void CAtspiRegistryService::unsubscribeSignal(GDBusConnection *connection, CAtspiRegistryService::RegistrationID id)
{
    g_dbus_connection_signal_unsubscribe(connection, static_cast<guint>(id));
}

void CAtspiRegistryService::registerDBusInterfaces(GDBusConnection *connection)
{
    GDBusInterfaceVTable vtable = {CAtspiRegistryService::onDBusMethodCallWrapper, NULL, NULL, {0}};

    auto id = registerInterface(connection, "/org/freedesktop/DBus", g_dbus_introspection_xml, &vtable);
    registrationIDForDBusInterfaces(connection, id);
}

void CAtspiRegistryService::unregisterDBusInterfaces(GDBusConnection *connection)
{
    unregisterInterface(connection, registrationIDForDBusInterfaces(connection));
    registrationIDForDBusInterfaces(connection, REGISTRATION_ID_INVALID);
}

void CAtspiRegistryService::registerRegistryInterfaces(GDBusConnection *connection)
{
    GDBusInterfaceVTable vtable = {CAtspiRegistryService::onAtspi2RegistryMethodCallWrapper, NULL, NULL, {0}};

    auto id = registerInterface(connection, "/org/a11y/atspi/registry", g_registry_introspection_xml, &vtable);
    registrationIDForAtspi2RegistryInterfaces(connection, id);
}

void CAtspiRegistryService::unregisterRegistryInterfaces(GDBusConnection *connection)
{
    unregisterInterface(connection, registrationIDForAtspi2RegistryInterfaces(connection));
    registrationIDForAtspi2RegistryInterfaces(connection, REGISTRATION_ID_INVALID);
}

CAtspiRegistryService::RegistrationID CAtspiRegistryService::registrationIDForDBusInterfaces(GDBusConnection *connection) const
{
    auto it = m_dbus_registration_id.find(connection);
    return it != m_dbus_registration_id.end() ? it->second : REGISTRATION_ID_INVALID;
}

CAtspiRegistryService::RegistrationID CAtspiRegistryService::registrationIDForAtspi2RegistryInterfaces(GDBusConnection *connection) const
{
    auto it = m_registry_registration_id.find(connection);
    return it != m_registry_registration_id.end() ? it->second : REGISTRATION_ID_INVALID;
}

void CAtspiRegistryService::registrationIDForDBusInterfaces(GDBusConnection *connection, RegistrationID registration_id)
{
    if (registration_id != REGISTRATION_ID_INVALID)
    {
        g_assert(m_dbus_registration_id.count(connection) == 0);

        m_dbus_registration_id.emplace(connection, registration_id);
    }
    else
    {
        m_dbus_registration_id.erase(connection);
    }
}

void CAtspiRegistryService::registrationIDForAtspi2RegistryInterfaces(GDBusConnection *connection, RegistrationID registration_id)
{
    if (registration_id != REGISTRATION_ID_INVALID)
    {
        g_assert(m_registry_registration_id.count(connection) == 0);

        m_registry_registration_id.emplace(connection, registration_id);
    }
    else
    {
        m_registry_registration_id.erase(connection);
    }
}

void CAtspiRegistryService::onDBusMethodCallWrapper(GDBusConnection *connection,
                                                    const gchar *sender,
                                                    const gchar *object_path,
                                                    const gchar *interface_name,
                                                    const gchar *method_name,
                                                    GVariant *parameters,
                                                    GDBusMethodInvocation *invocation,
                                                    gpointer user_data)
{
    CAtspiRegistryService *instance = static_cast<CAtspiRegistryService *>(user_data);

    return instance->onMethodCall(instance->m_dbus_method_handlers, connection, method_name, parameters, invocation);
}

void CAtspiRegistryService::onAtspi2RegistryMethodCallWrapper(GDBusConnection *connection,
                                                              const gchar *sender,
                                                              const gchar *object_path,
                                                              const gchar *interface_name,
                                                              const gchar *method_name,
                                                              GVariant *parameters,
                                                              GDBusMethodInvocation *invocation,
                                                              gpointer user_data)
{
    CAtspiRegistryService *instance = static_cast<CAtspiRegistryService *>(user_data);

    return instance->onMethodCall(instance->m_registry_method_handlers, connection, method_name, parameters, invocation);
}

void CAtspiRegistryService::onMethodCall(const DBusMethodCallHandlerMap &handlers,
                                         GDBusConnection *connection,
                                         const gchar *method_name,
                                         GVariant *parameters,
                                         GDBusMethodInvocation *invocation)
{
    RDKLOG_VERBOSE("Method handler: name = %s", method_name);

    if (auto it = handlers.find(method_name); it != handlers.end())
    {
        std::invoke(it->second, this, connection, parameters, invocation);
    }
    else
    {
        RDKLOG_ERROR("Unsupported method '%s'", method_name);
        g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Not implemented");
    }
}

void CAtspiRegistryService::handleDBusHello(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", CDBusClientRegistry::getByConnection(connection)->id().c_str()));
}

void CAtspiRegistryService::handleDBusStartServiceByName(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", 1));
}
void CAtspiRegistryService::handleDBusGetNameOwner(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    gchar *name;

    g_variant_get(parameters, "(&s)", &name);

    if (std::string(name) == "org.a11y.atspi.Registry")
    {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", CDBusClient::SERVER_ID.c_str()));
    }
    else
    {
        RDKLOG_WARNING("Owner of name '%s' not tracked.", name);
        g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NameHasNoOwner", "Not implemented");
    }
}

void CAtspiRegistryService::handleDBusAddMatch(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    // Unused
    (void)parameters;

    g_dbus_method_invocation_return_value(invocation, NULL);
}

void CAtspiRegistryService::handleDBusRemoveMatch(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    // Unused
    (void)parameters;

    g_dbus_method_invocation_return_value(invocation, NULL);
}

void CAtspiRegistryService::handleDBusRequestName(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    gchar *name;
    guint flags;

    g_variant_get(parameters, "(&su)", &name, &flags);

    auto client = CDBusClientRegistry::getByName(name);

    // TODO: We are not properly handling the name management here as we are ignoring the flags. Confirm this is not
    // an issue with multiple clients
    if (client)
    {
        RDKLOG_WARNING("Name '%s' already has an owner", name);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", DBUS_REQUEST_NAME_REPLY_EXISTS));
    }
    else
    {
        CDBusClientRegistry::getByConnection(connection)->name(name);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER));
    }
}
void CAtspiRegistryService::handleAtspi2GetRegisteredEvents(GDBusConnection *connection, GVariant *parameters, GDBusMethodInvocation *invocation)
{
    // Unused
    (void)parameters;

    GVariantBuilder builder;

    auto bus_id = CDBusClientRegistry::getByConnection(connection)->id().c_str();

    g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ss)"));
    for (auto const &entry : m_event_listeners)
    {
        g_variant_builder_add(&builder, "(ss)", bus_id, entry.first.c_str());
    }

    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new("(a(ss))", &builder));
}

void CAtspiRegistryService::onSignalWrapper(GDBusConnection *connection,
                                            const gchar *sender,
                                            const gchar *object_path,
                                            const gchar *interface_name,
                                            const gchar *signal_name,
                                            GVariant *parameters,
                                            gpointer user_data)
{
    CAtspiRegistryService *instance = static_cast<CAtspiRegistryService *>(user_data);

    // Get something in the form of "Document:LoadComplete"
    std::string event = CAtspiRegistryService::joinInterfaceAndSignal(
        CAtspiRegistryService::removeEventInterface(std::string(interface_name)), signal_name);

    gchar *name = NULL;
    guint d1 = 0, d2 = 0;
    std::string detail;
    std::string param_type(g_variant_get_type_string(parameters));

    // The below type is the only one currently supported and needed
    if (param_type == "(siiva{sv})")
    {
        g_variant_get(parameters, "(&siiva{sv})", &name, &d1, &d2, NULL, NULL);
        // Example data: ('focused', 0, 0, <'0'>, @a{sv} {})

        detail = name;
    }
    g_assert(param_type == "(siiva{sv})");

    instance->onSignal(connection, object_path, event, detail, d1, d2);
}
void CAtspiRegistryService::onSignal(GDBusConnection *connection,
                                     const std::string object_path,
                                     const std::string event,
                                     const std::string detail,
                                     uint32_t data1,
                                     uint32_t data2)
{
    RDKLOG_VERBOSE("Signal handler: path = %s event = %s detail = %s data1 = %u data2 = %u",
                   object_path.c_str(),
                   event.c_str(),
                   detail.c_str(),
                   data1,
                   data2);

    auto it = m_event_listeners.find(event);

    g_assert(it != m_event_listeners.end());

    CAtspiAccessibleObject object(connection, object_path);

    it->second.listener(event, detail, data1, data2, object);
}

void CAtspiRegistryService::subscribeSignalsPending()
{
    // Subscribe to event on active connections
    CDBusClientRegistry::execute([this](const CDBusClient &client)
                                 {
                                     for (auto &entry : m_event_listeners)
                                     {
                                         if (entry.second.registration_ids.count(client.connection()) != 0)
                                         {
                                             // Signal already subscribed for this connection
                                             continue;
                                         }
                                         auto event_split = splitInterfaceAndSignal(entry.first);

                                         auto id = subscribeSignal(client.connection(),
                                                                   addEventInterface(event_split.first),
                                                                   event_split.second,
                                                                   CAtspiRegistryService::onSignalWrapper);

                                         g_assert(id != REGISTRATION_ID_INVALID);
                                         if (id == REGISTRATION_ID_INVALID)
                                         {
                                             RDKLOG_WARNING("Unable to subscribe to signal '%s'", entry.first.c_str());
                                             continue;
                                         }
                                         entry.second.registration_ids.emplace(client.connection(), id);
                                     } });
}

void CAtspiRegistryService::unsubscribeSignalsActive(GDBusConnection *connection)
{
    for (auto &entry : m_event_listeners)
    {
        if (auto it = entry.second.registration_ids.find(connection); it != entry.second.registration_ids.end())
        {
            unsubscribeSignal(connection, it->second);
            entry.second.registration_ids.erase(it);
        }
    }
}

void CAtspiRegistryService::registerEventListener(const std::string &event, EventListener listener)
{
    g_assert(m_event_listeners.count(event) == 0);

    m_event_listeners.emplace(event, EventListenerInfo({.listener = listener}));

    subscribeSignalsPending();

    // This may fail if there is no active connection, but that is ok. When the connection is established,
    // peer can call the GetRegisteredEvents to get the list
    CDBusClientRegistry::execute([this, event](const CDBusClient &client)
                                 {
                                     GVariantBuilder empty;
                                     g_variant_builder_init(&empty, G_VARIANT_TYPE("as"));

                                     emitSignal(client.connection(),
                                                "/org/a11y/atspi/registry",
                                                "org.a11y.atspi.Registry",
                                                "EventListenerRegistered",
                                                g_variant_new("(ssas)", CDBusClient::SERVER_ID.c_str(), event.c_str(), &empty)); //
                                 });
}

void CAtspiRegistryService::unregisterEventListener(const std::string &event)
{
    DBusConnectionRegistrationIDsMap registration_ids;

    auto it = m_event_listeners.find(event);

    g_assert(it != m_event_listeners.end());

    registration_ids = it->second.registration_ids;

    m_event_listeners.erase(it);

    for (auto const &entry : registration_ids)
    {
        unsubscribeSignal(entry.first, entry.second);
    }

    // This may fail if there is no active connection, but that is ok as nobody will care for it in such case anyway
    CDBusClientRegistry::execute([this, event](const CDBusClient &client)
                                 {
                                     emitSignal(client.connection(),
                                                "/org/a11y/atspi/registry",
                                                "org.a11y.atspi.Registry",
                                                "EventListenerDeregistered",
                                                g_variant_new("(ss)", CDBusClient::SERVER_ID.c_str(), event.c_str())); //
                                 });
}
