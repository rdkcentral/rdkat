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
#include <vector>

/**
 * Interface for an accessible object
 * 
 * This provides access to an accessible object's properties such as name and description with
 * a standardized interface that abstracts away implementation details.
 * The properties provided are mostly based on the AT-SPI2 org.a11y.atspi.Accessible interface 
 * specification, although some may have been derived from traversing a tree of objects (e.g.
 * tables / cells) 
 */
class IAtspiAccessibleObject
{
public:
    /** A role indicating an error condition, such as uninitialized Role data */
    static inline constexpr uint32_t ATSPI_ROLE_INVALID = 0;
    /** An object the user can manipulate to tell the application to do something */
    static inline constexpr uint32_t ATSPI_ROLE_BUTTON = 43;
    /** An object used to represent information in terms of rows and columns */
    static inline constexpr uint32_t ATSPI_ROLE_TABLE = 55;
    /** A ‘cell’ or discrete child within a Table */
    static inline constexpr uint32_t ATSPI_ROLE_TABLE_CELL = 56;
    /** A row within a Table */
    static inline constexpr uint32_t ATSPI_ROLE_TABLE_ROW = 90;
    /** The object is a visual frame or container which contains a view of document content */
    static inline constexpr uint32_t ATSPI_ROLE_DOCUMENT_FRAME = 82;

    /** Indicates an invalid state - probably an error condition */
    static inline constexpr uint32_t ATSPI_STATE_INVALID = 0;
    /** Indicates this object is currently checked (e.g. checkbox element) */
    static inline constexpr uint32_t ATSPI_STATE_CHECKED = 4;

public:
    /**
     * Provides the accessible object's name
     *
     * @return The object's name
     */
    virtual std::string name() = 0;

    /**
     * Provides the accessible object's description
     *
     * @return The object's description
     */
    virtual std::string description() = 0;

    /**
     * Provides the accessible object's role name
     *
     * @return The object's role name
     */
    virtual std::string roleName() = 0;

    /**
     * Provides the accessible object's cell description (applicable to tables only)
     *
     * @return The object's cell description
     */
    virtual std::string cellDescription() = 0;

    /**
     * Provides the accessible object's role identifier
     *
     * @return The object's role identifier
     */
    virtual uint32_t role() = 0;

    /**
     * Provides the accessible object's states
     *
     * @return The object's states
     */
    virtual std::vector<uint32_t> states() = 0;

    /**
     * Provides the accessible object's object path
     *
     * @return The object's object path
     */
    virtual std::string objectPath() = 0;

    /**
     * Destructor
     */
    virtual ~IAtspiAccessibleObject() {}
};