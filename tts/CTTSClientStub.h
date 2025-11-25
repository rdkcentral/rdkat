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

#include <iostream>

#include "ITTSClient.h"

/**
 * Stub implementation for the ITTSClient interface that just prints text that shall be spoken
 */
class CTTSClientStub : public ITTSClient
{
public:
    /**
     * Constructor
     */
    CTTSClientStub() : m_initialized(false) {}

    /**
     * Destructor
     */
    ~CTTSClientStub() {}

    virtual void speak(const std::string &text)
    {
        std::cout << "TTS: Speaking text '" << text << "'" << std::endl;
    }
    virtual bool initialize()
    {
        if (!m_initialized)
        {
            m_initialized = true;
        }
        return true;
    }
    virtual void uninitialize()
    {
        if (m_initialized)
        {
            m_initialized = false;
        }
    }
    virtual void registerListener(StateChangeListener listener)
    {
        m_listener = listener;

        if (m_initialized)
        {
            // Notify TTS is enabled right away.
            m_listener(true);
        }
    }
    virtual void unregisterListener()
    {
        m_listener = nullptr;
    }

    virtual bool isEnabled()
    {
        return m_initialized;
    }

private:
    /** Listener for TTS state changes */
    StateChangeListener m_listener;

    /** Helper flag to track if the TTS Client is initialized  */
    bool m_initialized;
};
