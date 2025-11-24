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

#ifndef RDK_AT_ATSPI2_H
#define RDK_AT_ATSPI2_H

#ifdef  __cplusplus
extern "C"
{
#endif
    /**
     * Initializes the RDK-AT component
     *
     * Once initialized, accessibility related data providers (e.g. applications) will be able to
     * connect to the registry service and interact with it following the AT-SPI2 protocol (uses
     * D-Bus). The internal screen reader will act on the information and events to produce speech
     * based on the accessibility related information. To produce the speech, the TTSClient is
     * used.
     *
     * @return The AT-SPI2 address on which the service is running. The returned pointer shall not
     *         be released as it is owned by the RDKAT component.
     */
    const char * RDKAT_Initialize();

    /**
     * Uninitializes the RDK-AT component
     *
     * This will shutdown the component, terminating any client connection and not allow any future
     * connections.
     */
    void RDKAT_Uninitialize();
#ifdef  __cplusplus
}
#endif

#endif // RDK_AT_ATSPI2_H
