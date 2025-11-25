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

#include "CFactory.h"

#include "atspi2/CAtspiRegistryService.h"
#include "tts/CTTSClientStub.h"
#include "tts/CTTSClientThunder.h"
#include "screenreader/CScreenReader.h"

ITTSClient *CFactory::createTTSClient()
{
    return new CTTSClientThunder();
}

void CFactory::destroyTTSClient(ITTSClient *client)
{
    delete client;
}

IAtspiRegistryService *CFactory::createAtspiRegistryService()
{
    const gchar *address = g_getenv("AT_SPI_BUS_ADDRESS");
    if (!address)
    {
        char path_template[] = "/tmp/rdkat.XXXXXX";

        char * path = mkdtemp(path_template);

        std::string address_default = std::string("unix:path=") + std::string(path) + std::string("/socket");

        return new CAtspiRegistryService(address_default);
    }
    else
    {
        return new CAtspiRegistryService(address);
    }
}

void CFactory::destroyAtspiRegistryService(IAtspiRegistryService *service)
{
    delete service;
}

IScreenReader *CFactory::createScreenReader(IAtspiRegistryService *service, ITTSClient * client)
{
    return new CScreenReader(service, client);
}

void CFactory::destroyScreenReader(IScreenReader *screen_reader)
{
    delete screen_reader;
}
