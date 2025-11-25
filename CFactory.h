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

#include "tts/ITTSClient.h"
#include "atspi2/IAtspiRegistryService.h"
#include "screenreader/IScreenReader.h"

/**
 * Factory class with methods to create instances supporting ITTSClient, IAtspiRegistryService and
 * IScreenReader interfaces
 */
class CFactory
{
public:
    /**
     * Create a TTSClient instance
     *
     * @return Instance implementing ITTSClient interface
     */
    static ITTSClient *createTTSClient();

    /**
     * Destroy a TTSClient instance, releasing its resources
     *
     * @param [in] client The instance to destroy
     */
    static void destroyTTSClient(ITTSClient *client);

    /**
     * Create a AtspiRegistryService instance
     *
     * @return Instance implementing IAtspiRegistryService interface
     */
    static IAtspiRegistryService *createAtspiRegistryService();

    /**
     * Destroy a AtspiRegistryService instance, releasing its resources
     *
     * @param [in] service The instance to destroy
     */
    static void destroyAtspiRegistryService(IAtspiRegistryService *service);

    /**
     * Create a ScreenReader instance
     *
     * @return Instance implementing IScreenReader interface
     */
    static IScreenReader *createScreenReader(IAtspiRegistryService *service, ITTSClient *client);

    /**
     * Destroy a ScreenReader instance, releasing its resources
     *
     * @param [in] screen_reader The instance to destroy
     */
    static void destroyScreenReader(IScreenReader *screen_reader);
};