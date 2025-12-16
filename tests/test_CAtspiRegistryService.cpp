#include <gio/gio.h>
#include <glib.h>
#include <thread>

#include "gtest/gtest.h"

#include "CAtspiRegistryService.h"

#include "logger.h"

namespace CAtspiRegistryServiceTests
{

#define BITMASK(v) ((v) ? (1 << (v)) : 0)

static const gchar g_dbus_accessible_xml[] =
    "<node>"
    "  <interface name=\"org.a11y.atspi.Accessible\">"
    "    <property name=\"Name\" type=\"s\" access=\"read\"/>"
    "    <property name=\"Description\" type=\"s\" access=\"read\"/>"
    "   <property name=\"Parent\" type=\"(so)\" access=\"read\">"
    "     <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"QSpiObjectReference\"/>"
    "   </property>"
    "    <method name=\"GetRole\">"
    "      <arg direction=\"out\" type=\"u\"/>"
    "    </method>"
    "    <method name=\"GetRoleName\">"
    "      <arg direction=\"out\" type=\"s\"/>"
    "    </method>"
    "    <method name=\"GetState\">"
    "      <arg direction=\"out\" type=\"au\"/>"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QSpiIntList\"/>"
    "    </method>"
    "  </interface>"
    "</node>";

static const gchar g_dbus_table_xml[] =
    "<node>"
    "  <interface name=\"org.a11y.atspi.Table\">"
    "    <property name=\"Caption\" type=\"(so)\" access=\"read\">"
    "      <annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"QSpiObjectReference\"/>"
    "    </property>"
    "  </interface>"
    "</node>";

typedef struct
{
    std::string name;
    std::string description;
    std::string role_name;
    uint32_t role;
    std::vector<uint32_t> states;
    std::string object_path;
    std::string parent_object_path;
    std::string expected_cell_description;
} TTestParamAccessible;

typedef struct
{
    std::string event;
    std::string detail;
    uint32_t data1;
    std::vector<TTestParamAccessible> accessible;
} TTestParam;

class CAtspiRegistryServiceTest : public ::testing::TestWithParam<TTestParam>
{
protected:
    CAtspiRegistryServiceTest() = default;
    virtual ~CAtspiRegistryServiceTest() = default;

    virtual void SetUp()
    {
        setupMainLoop();
    }

    virtual void TearDown()
    {
        teardownMainLoop();
    }

public:
    void setupMainLoop()
    {
        m_default_main_loop = g_main_loop_new(nullptr, FALSE);

        ASSERT_NE(nullptr, m_default_main_loop);

        m_default_main_thread = std::thread(
            [](GMainLoop *loop)
            {
                g_main_loop_run(loop);
            },
            m_default_main_loop);
    }
    void teardownMainLoop()
    {
        ASSERT_NE(nullptr, m_default_main_loop);

        stopMainLoop();

        m_default_main_thread.join();
        g_main_loop_unref(m_default_main_loop);
        
        m_default_main_loop = nullptr;
    }
    void stopMainLoop()
    {
        g_main_loop_quit(m_default_main_loop);
    }
    void stopWorkerLoop()
    {
        g_main_loop_quit(m_worker_loop);
    }
    void deferConnectionClose(GDBusConnection *connection)
    {
        m_connection = connection;
    }
    CAtspiRegistryService *service()
    {
        return m_service;
    }
    GMainContext *context()
    {
        return m_context;
    }

    void registerInterface(GDBusConnection *connection,
                               const std::string &object_path,
                               const std::string &interface_xml,
                               const GDBusInterfaceVTable *vtable)
    {
        g_assert(connection != nullptr);

        GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(interface_xml.c_str(), NULL);
        if (introspection_data == nullptr)
        {
            ADD_FAILURE() << "Unable to create DBus introspection data";
            return;
        }

        guint reg_id = g_dbus_connection_register_object(connection,
                                                         object_path.c_str(),
                                                         introspection_data->interfaces[0],
                                                         vtable, /* Will be copied */
                                                         this,   /* user_data */
                                                         NULL,   /* user_data_free_func */
                                                         NULL);  /* GError** */

        g_dbus_node_info_unref(introspection_data);

        if (!reg_id)
        {
            ADD_FAILURE() << "Unable to register object on DBus connection";
        }
    }

    void registerDBusInterfaces(GDBusConnection *connection)
    {
        GDBusInterfaceVTable vtable = {CAtspiRegistryServiceTest::onDBusMethodCallWrapper, CAtspiRegistryServiceTest::onDBusGetPropertyCallWrapper, NULL, {0}};
        auto & param = GetParam();

        for (auto & accessible : param.accessible)
        {
            switch(accessible.role)
            {
                case IAtspiAccessibleObject::ATSPI_ROLE_TABLE:
                case IAtspiAccessibleObject::ATSPI_ROLE_TABLE_ROW:
                case IAtspiAccessibleObject::ATSPI_ROLE_TABLE_CELL:
                {
                    registerInterface(connection, accessible.object_path, g_dbus_table_xml, &vtable);
                    // Fall through as we need the accessible interface as well
                }
                default:
                {
                    registerInterface(connection, accessible.object_path, g_dbus_accessible_xml, &vtable);
                }
            }
        }
    }

public:
    static void onDBusMethodCallWrapper(GDBusConnection *connection,
                                        const gchar *sender,
                                        const gchar *object_path,
                                        const gchar *interface_name,
                                        const gchar *method_name,
                                        GVariant *parameters,
                                        GDBusMethodInvocation *invocation,
                                        gpointer user_data)
    {
        CAtspiRegistryServiceTest *instance = static_cast<CAtspiRegistryServiceTest *>(user_data);

        return instance->onMethodCall(connection, object_path, method_name, parameters, invocation);
    }

    static GVariant * onDBusGetPropertyCallWrapper(GDBusConnection *connection,
                                                const gchar *sender,
                                                const gchar *object_path,
                                                const gchar *interface_name,
                                                const gchar *property_name,
                                                GError** error,
                                                gpointer user_data)
    {
        CAtspiRegistryServiceTest *instance = static_cast<CAtspiRegistryServiceTest *>(user_data);

        return instance->onGetPropertyCall(connection, object_path, property_name, error);
    }

protected:
    const TTestParamAccessible * getTestParamAccessibleObject(std::string object_path)
    {
        auto & param = GetParam();

        auto it = std::find_if(param.accessible.begin(), param.accessible.end(),
            [&](const TTestParamAccessible& accessible){ return accessible.object_path == object_path; });

        return (it != param.accessible.end()) ? &(*it) : nullptr;
    }

    const TTestParamAccessible * getTestParamAccessibleObject(uint32_t role)
    {
        auto & param = GetParam();

        auto it = std::find_if(param.accessible.begin(), param.accessible.end(),
            [&](const TTestParamAccessible& accessible){ return accessible.role == role; });

        return (it != param.accessible.end()) ? &(*it) : nullptr;
    }

    void onMethodCall(GDBusConnection *connection,
                      const gchar *object_path,
                      const gchar *method_name,
                      GVariant *parameters,
                      GDBusMethodInvocation *invocation)
    {
        auto accessible = getTestParamAccessibleObject(object_path);

        if (!accessible)
        {
            g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.UnknownObject", "Not found");
            return;
        }

        if (!g_strcmp0(method_name, "GetRole"))
        {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", accessible->role));
        }
        else if (!g_strcmp0(method_name, "GetRoleName"))
        {
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", accessible->role_name.c_str()));
        }
        else if (!g_strcmp0(method_name, "GetState")) {
            GVariantBuilder builder = G_VARIANT_BUILDER_INIT(G_VARIANT_TYPE("(au)"));

            g_variant_builder_open(&builder, G_VARIANT_TYPE("au"));

            uint64_t states_mask = 0;
            for (auto state : accessible->states)
            {
                states_mask |=  1 << state;
            }
            g_variant_builder_add(&builder, "u", static_cast<uint32_t>(states_mask & 0xffffffff));
            g_variant_builder_add(&builder, "u", static_cast<uint32_t>(states_mask >> 32));
            
            g_variant_builder_close(&builder);

            g_dbus_method_invocation_return_value(invocation, g_variant_builder_end(&builder));
        }
        else
        {
            g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.NotSupported", "Not implemented");
        }
    }

    GVariant * onGetPropertyCall(GDBusConnection *connection,
                                 const gchar *object_path,
                                 const gchar *property_name,
                                 GError** error)
    {
        auto accessible = getTestParamAccessibleObject(object_path);

        if (!accessible)
        {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Object '%s' not found", object_path);
            return nullptr;
        }

        if (!g_strcmp0(property_name, "Name"))
        {
            return g_variant_new_string(accessible->name.c_str());
        }
        if (!g_strcmp0(property_name, "Description"))
        {
            return g_variant_new_string(accessible->description.c_str());
        }
        if (!g_strcmp0(property_name, "Parent"))
        {
            const gchar * unique_name = g_dbus_connection_get_unique_name(connection);
            if (!unique_name)
            {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Unable to get unique name for connection");
                return nullptr;
            }
            return g_variant_new("(so)", unique_name, accessible->parent_object_path.c_str());
        }
        if (!g_strcmp0(property_name, "Caption"))
        {
            const gchar * unique_name = g_dbus_connection_get_unique_name(connection);
            if (!unique_name)
            {
                g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Unable to get unique name for connection");
                return nullptr;
            }
            
            auto caption = getTestParamAccessibleObject(IAtspiAccessibleObject::ATSPI_ROLE_CAPTION);

            std::string object_path = caption ? caption->object_path : "/org/a11y/atspi/null";

            return g_variant_new("(so)", unique_name, object_path.c_str());
        }

        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown property '%s'", property_name);
        return nullptr;
    }

protected:
    GDBusConnection *m_connection = { nullptr };
    GMainContext *m_context = { nullptr };
    GMainLoop *m_worker_loop = { nullptr };
    GMainLoop *m_default_main_loop = { nullptr };
    std::thread m_default_main_thread;
    CAtspiRegistryService *m_service = { nullptr };
};

#define INVOKE(self, code)                                                        \
    {                                                                             \
        /* Call on worker context */                                              \
        g_main_context_invoke(                                                    \
            self->context(),                                                      \
            [](gpointer user_data) -> gboolean {                                  \
                auto* test = static_cast<CAtspiRegistryServiceTest *>(user_data); \
                code;                                                             \
                return G_SOURCE_REMOVE;                                           \
            },                                                                    \
            self);                                                                \
    }

#define INVOKE_CLIENT(self, code)                                                 \
    {                                                                             \
        /* Call on main/default context */                                        \
        g_main_context_invoke(                                                    \
            nullptr,                                                              \
            [](gpointer user_data) -> gboolean {                                  \
                auto* test = static_cast<CAtspiRegistryServiceTest *>(user_data); \
                code;                                                             \
                return G_SOURCE_REMOVE;                                           \
            },                                                                    \
            self);                                                                \
    }

#define INVOKE_TIMEOUT(self, code, delay_ms)                                      \
    {                                                                             \
        /* Call on main context */                                                \
        g_timeout_add(                                                            \
            delay_ms,                                                             \
            [](gpointer user_data) -> gboolean {                                  \
                auto* test = static_cast<CAtspiRegistryServiceTest *>(user_data); \
                INVOKE(test, code); /* Call on worker context */                  \
                return G_SOURCE_REMOVE;                                           \
            },                                                                    \
            self);                                                                \
    }

TEST_P(CAtspiRegistryServiceTest, events)
{
    const ::testing::TestInfo *test_info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string address = std::string("unix:abstract=") + std::string(test_info->test_case_name()) + "/" + std::string(test_info->name());
       
    m_context = g_main_context_new();
    m_worker_loop = g_main_loop_new(m_context, FALSE);
    g_main_context_push_thread_default(m_context);

    m_service = new CAtspiRegistryService(address);

    ASSERT_NE(nullptr, m_service);

    // Timeout test
    INVOKE_TIMEOUT(
        this,
        {
            test->service()->stop();
            test->stopWorkerLoop();
            ADD_FAILURE() << "Test case timed out";
        },
        2000);

    INVOKE(
        this,
        {
            auto & param = test->GetParam();

            test->service()->registerEventListener(
                param.event,
                [test](const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object) -> void
                {
                    auto & param = test->GetParam();

                    EXPECT_STREQ(event.c_str(), param.event.c_str());
                    EXPECT_STREQ(detail.c_str(), param.detail.c_str());
                    EXPECT_EQ(data1, param.data1);

                    EXPECT_STREQ(object.name().c_str(), param.accessible[0].name.c_str());
                    EXPECT_STREQ(object.description().c_str(), param.accessible[0].description.c_str());
                    EXPECT_STREQ(object.roleName().c_str(), param.accessible[0].role_name.c_str());
                    EXPECT_EQ(object.role(), param.accessible[0].role);
                    EXPECT_EQ(param.accessible[0].states, object.states()); // Assume ordered
                    EXPECT_STREQ(object.cellDescription().c_str(), param.accessible[0].expected_cell_description.c_str());
                    EXPECT_STREQ(object.objectPath().c_str(), param.accessible[0].object_path.c_str());

                    // Finish test
                    INVOKE(test, {
                        test->service()->stop();
                        test->stopWorkerLoop();
                    });
                });
            test->service()->start();
        });

    // Connect client to send event to service and listen for requests
    INVOKE_CLIENT(
        this,
        {
            g_dbus_connection_new_for_address(
                test->service()->address().c_str(),
                static_cast<GDBusConnectionFlags>(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
                nullptr,
                nullptr,
                [](GObject *, GAsyncResult *result, gpointer user_data)
                {
                    auto test = static_cast<CAtspiRegistryServiceTest *>(user_data);
                    auto & param = test->GetParam();
                    GError *error = nullptr;

                    GDBusConnection *connection = g_dbus_connection_new_for_address_finish(result, &error);
                    if (error)
                    {
                        ADD_FAILURE() << "Failed to connect to registry. Error: " << error->message;
                        g_clear_error(&error);
                        return;
                    }

                    test->registerDBusInterfaces(connection);

                    // convert "Document:LoadComplete" style event into "org.a11y.atspi.Event.Document" and "LoadComplete"
                    auto [target, subevent] = [&]() {
                        const std::string& event = param.event;
                        size_t pos = event.find(":");
                        return std::pair{"org.a11y.atspi.Event." + event.substr(0, pos), event.substr(pos + 1)};
                    }();                    
                    g_dbus_connection_emit_signal(
                        connection,
                        NULL,
                        param.accessible[0].object_path.c_str(),
                        target.c_str(),
                        subevent.c_str(),
                        g_variant_new("(siiva{sv})", param.detail.c_str(), param.data1, 0, g_variant_new_string(""), nullptr),
                        &error);

                    if (error)
                    {
                        ADD_FAILURE() << "Failed to send signal to registry. Error: " << error->message;
                        g_clear_error(&error);
                    }
                    // Do not unref connection here, as it may cause a disconnection before the signal reaches the service
                    // Do it on cleanup
                    test->deferConnectionClose(connection);
                },
                user_data);
        });

    g_main_loop_run(m_worker_loop);

    if (m_connection)
    {
        g_object_unref(m_connection);
        m_connection = nullptr;
    }

    delete m_service;

    g_main_context_pop_thread_default(m_context);
    g_main_loop_unref(m_worker_loop);
    g_main_context_unref(m_context);
}

INSTANTIATE_TEST_SUITE_P(
    RegistryServiceEventsTest,
    CAtspiRegistryServiceTest,
    ::testing::Values(
        TTestParam{
            "Document:LoadComplete",
            "",
            0,
            {{"name", "description", "document web", IAtspiAccessibleObject::ATSPI_ROLE_DOCUMENT_WEB, {}, "/org/a11y/atspi/accessible/root", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {{"name", "description", "button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            0,
            {{"name", "description", "button", IAtspiAccessibleObject::ATSPI_ROLE_PUSH_BUTTON, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {{"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {{"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            0,
            {{"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {{"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {{"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, { IAtspiAccessibleObject::ATSPI_STATE_CHECKED }, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            0,
            {{"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "checked",
            1,
            {{"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "checked",
            0,
            {{"name", "description", "check", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "checked",
            1,
            {{"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },
        TTestParam{
            "Object:StateChanged",
            "checked",
            0,
            {{"name", "description", "check box", IAtspiAccessibleObject::ATSPI_ROLE_CHECK_BOX, {}, "/org/a11y/atspi/accessible/1", "", ""}}
        },

        // Table objects. Test param 'expected_cell_description' field contains the expected result for the accessible 'cellDescription' property calculated dynamically
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {
                {"cell name", "cell description", "table cell", IAtspiAccessibleObject::ATSPI_ROLE_TABLE_CELL, {}, "/org/a11y/atspi/accessible/table/row/cell", "/org/a11y/atspi/accessible/table/row", "caption name.row name."},
                {"row name", "row description", "table row", IAtspiAccessibleObject::ATSPI_ROLE_TABLE_ROW, {}, "/org/a11y/atspi/accessible/table/row", "/org/a11y/atspi/accessible/table", ""},
                {"table name", "table description", "table", IAtspiAccessibleObject::ATSPI_ROLE_TABLE, {}, "/org/a11y/atspi/accessible/table", "/org/a11y/atspi/accessible/root", ""},
                {"caption name", "caption description", "caption", IAtspiAccessibleObject::ATSPI_ROLE_CAPTION, {}, "/org/a11y/atspi/accessible/table/caption", "/org/a11y/atspi/accessible/table", ""}
            }
        },
        TTestParam{
            "Object:StateChanged",
            "focused",
            1,
            {
                {"cell name", "cell description", "table cell", IAtspiAccessibleObject::ATSPI_ROLE_TABLE_CELL, {}, "/org/a11y/atspi/accessible/table/row/cell", "/org/a11y/atspi/accessible/table/row", "row name."},
                {"row name", "row description", "table row", IAtspiAccessibleObject::ATSPI_ROLE_TABLE_ROW, {}, "/org/a11y/atspi/accessible/table/row", "/org/a11y/atspi/accessible/table", ""},
                {"table name", "table description", "table", IAtspiAccessibleObject::ATSPI_ROLE_TABLE, {}, "/org/a11y/atspi/accessible/table", "/org/a11y/atspi/accessible/root", ""},
            }
        }
    )
);

} // namespace CAtspiRegistryServiceTests