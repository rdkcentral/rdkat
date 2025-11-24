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

#include "IAtspiAccessibleObject.h"

#include <gio/gio.h>
#include <glib.h>

/**
 * Concrete implementation of @ref IAtspiAccessibleObject using D-Bus
 */
class CAtspiAccessibleObject : public IAtspiAccessibleObject
{
public:
    /**
     * Constructs an instance of this class
     *
     * @param [in] connection  The D-Bus connection this instance is associated with
     * @param [in] object_path A path that uniquely identifies this object within the accessibile
     *                         objects tree of an accessible application
     */
    CAtspiAccessibleObject(GDBusConnection *connection, const std::string &object_path);

    virtual ~CAtspiAccessibleObject();

    virtual std::string name();
    virtual std::string description();
    virtual std::string roleName();
    virtual uint32_t role();
    virtual std::vector<uint32_t> states();
    virtual std::string cellDescription();
    virtual std::string objectPath();

private:
    /**
     * Returns a table cell's description build up on the table caption and accessible name of the
     * associated row element
     */
    std::string getCellDescription();

    /**
     * Get's an object's string property over D-Bus
     *
     * @param [in]  object_path The object path that uniquely identifies the object within the
     *                          accessible object tree
     * @param [in]  name        The name of the property to get
     * @param [out] output      The property value
     *
     * @return The property value
     *
     * @retval true  The property was successfully fetched and output variable was set accordingly
     * @retval false An error occurred and the property could not be fetched. The output param
     *               remains unchanged
     */
    bool dbusPropertyCallToClient(const std::string &object_path, const std::string &name, std::string &output);

    /**
     * Get's an object's name and object path property over D-Bus for the provided interface
     *
     * @param [in]  object_path The object path that uniquely identifies the object within the
     *                          accessible object tree
     * @param [in]  interface   The AT-SPI2 interface to use for this operation
     * @param [in]  name        The name of the property to get
     * @param [out] output_name The property value name field
     * @param [out] output_path The property value path field
     *
     * @return The property value name and path fields
     *
     * @retval true The property was successfully fetched and output variables were set accordingly
     * @retval false An error occurred and the property could not be fetched. The output params
     *               remain unchanged
     */
    bool atspiPropertyCallToClient(const std::string &object_path, const std::string &interface, const std::string &name, std::string &output_name, std::string &output_path);

    /**
     * Performs a method call over D-Bus on the remote object where the output is expected to be a
     * single string
     *
     * @param [in]  object_path The object path that uniquely identifies the object within the
     *                          accessible object tree
     * @param [in]  name        The name of the method to call
     * @param [out] output      The method output value
     *
     * @return The output of the method call
     *
     * @retval true  The method was successfully called and output variable was set accordingly
     * @retval false An error occurred when performing the call. The output param remains unchanged
     */
    bool atspiMethodCallToClient(const std::string &object_path, const std::string &name, std::string &output);

    /**
     * Performs a method call over D-Bus on the remote object where the output is expected to be a
     * single 32 bit integer
     *
     * @param [in]  object_path The object path that uniquely identifies the object within the
     *                          accessible object tree
     * @param [in]  name        The name of the method to call
     * @param [out] output      The method output value
     *
     * @return The output of the method call
     *
     * @retval true  The method was successfully called and output variable was set accordingly
     * @retval false An error occurred when performing the call. The output param remains unchanged
     */
    bool atspiMethodCallToClient(const std::string &object_path, const std::string &name, uint32_t &output);

    /**
     * Performs a method call over D-Bus on the remote object where the output is expected to be an
     * array of 32 bit integers
     *
     * @param [in]  object_path The object path that uniquely identifies the object within the
     *                          accessible object tree
     * @param [in]  name        The name of the method to call
     * @param [out] output      The method output value
     *
     * @return The output of the method call
     *
     * @retval true  The method was successfully called and output variable was set accordingly
     * @retval false An error occurred when performing the call. The output param remains unchanged
     */
    bool atspiMethodCallToClient(const std::string &object_path, const std::string &name, std::vector<uint32_t> &output);

private:
    /** The object path that uniquely identifies this object within the accessible object tree */
    std::string m_object_path;
    /** The D-Bus connection associated with this instance */
    GDBusConnection *m_connection;

    /** A cached value of the accessible object's name */
    std::string m_name;
    /** A cached value of the accessible object's description */
    std::string m_description;
    /** A cached value of the accessible object's role name */
    std::string m_role_name;
    /** A cached value of the accessible object's cell description */
    std::string m_cell_description;

    /** A cached value of the accessible object's role  */
    uint32_t m_role = ATSPI_ROLE_INVALID;
    /** A cached value of the accessible object's states */
    std::vector<uint32_t> m_states;
};