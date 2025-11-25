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

#include <glib.h>

#include "rdkat-atspi2.h"
#include "logger.h"
#include "CFactory.h"

static IAtspiRegistryService *g_service = nullptr;
static ITTSClient *g_tts_client = nullptr;
static IScreenReader *g_screen_reader = nullptr;

const char * RDKAT_Initialize()
{
    if (g_screen_reader == nullptr)
    {
        RDK_AT::logger_init();

        g_service = CFactory::createAtspiRegistryService();
        g_tts_client = CFactory::createTTSClient();
        g_screen_reader = CFactory::createScreenReader(g_service, g_tts_client);

        g_assert(g_service != nullptr);
        g_assert(g_tts_client != nullptr);
        g_assert(g_screen_reader != nullptr);

        g_screen_reader->initialize();
    }

    // static const ensures it is safe to return the internal pointer
    static const std::string address = g_service->address();

    return address.c_str();
}

void RDKAT_Uninitialize()
{
    if (g_screen_reader == nullptr)
    {
        // Already uninitialized
        return;
    }
    g_assert(g_service != nullptr);
    g_assert(g_tts_client != nullptr);

    // Ensure screen reader is disabled
    g_screen_reader->uninitialize();

    CFactory::destroyScreenReader(g_screen_reader);
    CFactory::destroyTTSClient(g_tts_client);
    CFactory::destroyAtspiRegistryService(g_service);

    g_screen_reader = nullptr;
    g_tts_client = nullptr;
    g_service = nullptr;
}
