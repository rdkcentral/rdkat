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

#include <gio/gio.h>
#include <glib.h>

#include "logger.h"
#include "CAtspiAccessibleObject.h"

CAtspiAccessibleObject::CAtspiAccessibleObject(GDBusConnection *connection, const std::string &object_path)
    : m_object_path(object_path), m_connection(g_object_ref(connection))
{
}

CAtspiAccessibleObject::~CAtspiAccessibleObject()
{
    g_object_unref(m_connection);
}

std::string CAtspiAccessibleObject::name()
{
    if (!m_name.empty())
    {
        // Use cached value, if available
        return m_name;
    }

    dbusPropertyCallToClient(m_object_path, "Name", m_name);

    return m_name;
}

std::string CAtspiAccessibleObject::description()
{
    if (!m_description.empty())
    {
        // Use cached value, if available
        return m_description;
    }

    dbusPropertyCallToClient(m_object_path, "Description", m_description);

    return m_description;
}

std::string CAtspiAccessibleObject::roleName()
{
    if (!m_role_name.empty())
    {
        // Use cached value, if available
        return m_role_name;
    }

    atspiMethodCallToClient(m_object_path, "GetRoleName", m_role_name);

    return m_role_name;
}

uint32_t CAtspiAccessibleObject::role()
{
    if (m_role != ATSPI_ROLE_INVALID)
    {
        // Use cached value, if available
        return m_role;
    }

    atspiMethodCallToClient(m_object_path, "GetRole", m_role);

    return m_role;
}

std::vector<uint32_t> CAtspiAccessibleObject::states()
{
    std::vector<uint32_t> states_fetched;

    if (!m_states.empty())
    {
        // Use cached value, if available
        return m_states;
    }
    atspiMethodCallToClient(m_object_path, "GetState", states_fetched);
    if (states_fetched.size() != 2)
    {
        // Return empty array (m_states was not initialized yet)
        return m_states;
    }
    uint64_t states_raw = (((uint64_t)states_fetched[1]) << 32) | states_fetched[0];

    for (uint32_t offset = 0; offset < (sizeof(states_raw) * 8); offset++)
    {
        if (states_raw & (1ull << offset))
            m_states.emplace_back(offset);
    }

    return m_states;
}

std::string CAtspiAccessibleObject::cellDescription()
{
    if (!m_cell_description.empty())
    {
        // Use cached value, if available
        return m_cell_description;
    }

    m_cell_description = getCellDescription();

    return m_cell_description;
}

std::string CAtspiAccessibleObject::objectPath()
{
    return m_object_path;
}

std::string CAtspiAccessibleObject::getCellDescription()
{
    static std::string saved_table_object_path;
    static std::string saved_row_object_path;
    static std::string saved_cell_object_path;

    std::string name;

    uint32_t role = this->role();

    // On focusing a non-cell element, forget previous cell details
    if (role != ATSPI_ROLE_TABLE_CELL)
    {
        saved_table_object_path.clear();
        saved_row_object_path.clear();
        saved_cell_object_path.clear();
        return "";
    }

    std::string table_object_path;
    std::string row_object_path;
    std::string cell_object_path = m_object_path;
    std::string object_path = m_object_path;

    // Find table object
    while (!object_path.empty())
    {
        if (!atspiPropertyCallToClient(object_path, "org.a11y.atspi.Accessible", "Parent", name, object_path))
        {
            RDKLOG_WARNING("Failed to get parent for object");
            return "";
        }
        if (object_path.empty())
        {
            RDKLOG_WARNING("Empty object path");
            break;
        }
        if (!atspiMethodCallToClient(object_path, "GetRole", role))
        {
            RDKLOG_WARNING("Failed to get role for object");
            return "";
        }
        if (role == ATSPI_ROLE_TABLE)
        {
            table_object_path = object_path;
            break;
        }
        if (role == ATSPI_ROLE_TABLE_ROW)
        {
            row_object_path = object_path;
        }
    }

    // Retrieve table caption
    std::string caption;
    if (!table_object_path.empty())
    {
        std::string caption_object_path;

        if (!atspiPropertyCallToClient(object_path, "org.a11y.atspi.Table", "Caption", name, caption_object_path))
        {
            RDKLOG_WARNING("Failed to get caption for object");
            return "";
        }
        if (!caption_object_path.empty() && ((cell_object_path == saved_cell_object_path) || (table_object_path != saved_table_object_path)))
        {
            dbusPropertyCallToClient(caption_object_path, "Name", caption);
        }
    }

    // Retrieve Row Heading
    // Note : this is not the not Row Header element, but the accessible name of Row element
    std::string row_header;
    if (!row_object_path.empty() && ((cell_object_path == saved_cell_object_path) || (row_object_path != saved_row_object_path)))
    {
        dbusPropertyCallToClient(row_object_path, "Name", row_header);
    }

    // Remember table information
    saved_table_object_path = table_object_path;
    saved_row_object_path = row_object_path;
    saved_cell_object_path = cell_object_path;

    std::string description;

    if (!caption.empty())
    {
        description = caption + ". ";
    }
    if (!row_header.empty())
    {
        description += row_header + ". ";
    }

    return description;
}

bool CAtspiAccessibleObject::dbusPropertyCallToClient(const std::string &object_path, const std::string &name, std::string &output)
{
    GVariant *value;
    GError *error = NULL;
    bool ret = false;

    value = g_dbus_connection_call_sync(m_connection,
                                        NULL, /* bus_name */
                                        object_path.c_str(),
                                        "org.freedesktop.DBus.Properties",
                                        "Get",
                                        g_variant_new("(ss)", "org.a11y.atspi.Accessible", name.c_str()),
                                        G_VARIANT_TYPE("(v)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (error)
    {
        RDKLOG_ERROR("Error invoking dbusPropertyCallToClient(): %s", error->message);
        g_error_free(error);
        return false;
    }
    GVariant *variant;

    g_variant_get(value, "(v)", &variant);

    // Other return type cases are not currently supported
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_STRING))
    {
        output = g_variant_get_string(variant, NULL);
        ret = true;
    }
    g_assert(g_variant_is_of_type(variant, G_VARIANT_TYPE_STRING));

    g_variant_unref(value);
    return ret;
}

bool CAtspiAccessibleObject::atspiPropertyCallToClient(const std::string &object_path, const std::string &interface, const std::string &name, std::string &output_name, std::string &output_path)
{
    GVariant *value;
    GError *error = NULL;
    bool ret = false;

    value = g_dbus_connection_call_sync(m_connection,
                                        NULL, /* bus_name */
                                        object_path.c_str(),
                                        "org.freedesktop.DBus.Properties",
                                        "Get",
                                        g_variant_new("(ss)", interface.c_str(), name.c_str()),
                                        G_VARIANT_TYPE("(v)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (error)
    {
        RDKLOG_ERROR("Error invoking atspiPropertyCallToClient(): %s", error->message);
        g_error_free(error);
        return false;
    }
    const gchar *name_str = NULL;
    const gchar *path_str = NULL;
    GVariant *variant;

    g_variant_get(value, "(v)", &variant);

    // Other return type cases are not currently supported
    if (g_variant_is_of_type(variant, (const GVariantType *)"(so)"))
    {
        g_variant_get(variant, "(&so)", &name_str, &path_str);

        output_name = std::string(name_str);
        output_path = std::string(path_str);

        ret = true;
    }

    g_variant_unref(value);

    return ret;
}

bool CAtspiAccessibleObject::atspiMethodCallToClient(const std::string &object_path, const std::string &name, std::string &output)
{
    GVariant *value;
    GError *error = NULL;

    value = g_dbus_connection_call_sync(m_connection,
                                        NULL, /* bus_name */
                                        object_path.c_str(),
                                        "org.a11y.atspi.Accessible",
                                        name.c_str(),
                                        NULL,
                                        G_VARIANT_TYPE("(s)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (error)
    {
        RDKLOG_ERROR("Error invoking atspiMethodCallToClient(): %s", error->message);
        g_error_free(error);
        return false;
    }
    const gchar *value_str = NULL;

    g_variant_get(value, "(&s)", &value_str);

    output = std::string(value_str);

    g_variant_unref(value);

    return true;
}

bool CAtspiAccessibleObject::atspiMethodCallToClient(const std::string &object_path, const std::string &name, uint32_t &output)
{
    GVariant *value;
    GError *error = NULL;

    value = g_dbus_connection_call_sync(m_connection,
                                        NULL, /* bus_name */
                                        object_path.c_str(),
                                        "org.a11y.atspi.Accessible",
                                        name.c_str(),
                                        NULL,
                                        G_VARIANT_TYPE("(u)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (error)
    {
        RDKLOG_ERROR("Error invoking atspiMethodCallToClient(): %s", error->message);
        g_error_free(error);
        return false;
    }
    guint32 value_uint = 0;

    g_variant_get(value, "(u)", &value_uint);

    output = value_uint;

    g_variant_unref(value);

    return true;
}

bool CAtspiAccessibleObject::atspiMethodCallToClient(const std::string &object_path, const std::string &name, std::vector<uint32_t> &output)
{
    GVariant *value;
    GError *error = NULL;

    value = g_dbus_connection_call_sync(m_connection,
                                        NULL, /* bus_name */
                                        object_path.c_str(),
                                        "org.a11y.atspi.Accessible",
                                        name.c_str(),
                                        NULL,
                                        G_VARIANT_TYPE("(au)"),
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);
    if (error)
    {
        RDKLOG_ERROR("Error invoking atspiMethodCallToClient(): %s", error->message);
        g_error_free(error);
        return false;
    }

    GVariant *array_value;
    const guint32 *array = NULL;
    gsize count;

    g_variant_get(value, "(@au)", &array_value);

    array = static_cast<const guint32 *>(g_variant_get_fixed_array(array_value,
                                                                   &count,
                                                                   sizeof(guint32)));
    g_assert(array != nullptr);

    output.clear();
    output.reserve(count);
    for (gsize i = 0; (array != nullptr), i < count; i++)
    {
        output.emplace_back(array[i]);
    }

    g_variant_unref(value);

    return true;
}
