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
#include <functional>

/**
 * Interface for Text-To-Speech service
 *
 * An implementation of this service shall provide the ability to transform text into audible
 * speech using the ITTSClient::speak method. The implementation shall also provide a notification
 * when TTS is enabled or disabled by the user by calling the listener registered via
 * ITTSClient::registerListener. State can also be queried using ITTSClient::isEnabled
 */
class ITTSClient
{
public:
    /**
     * Signature of the state change listener callback function
     *
     * This function provides information about TTS being enabled or disabled
     *
     * @param [in] enabled Indicates if TTS is enabled or disabled
     */
    using StateChangeListener = std::function<void(bool enabled)>;

public:
    /**
     * Transform the provided text into audible speech
     *
     * @param [in] text The text to transform into audible speech
     */
    virtual void speak(const std::string &text) = 0;

    /**
     * Initializes the TTS client implementation to be ready to transform text into speech and
     * notify about TTS state changes
     *
     * @returns Indication if initialization completed successfully
     *
     * @retval true  Successfully initialized implementation
     * @retval false An error occured and initialization did not complete
     *
     * @see ITTSClient::uninitialize
     */
    virtual bool initialize() = 0;

    /**
     * Unitializes the TTS client implementation, afer which no text to speech transformation
     * is possible or TTS state changes notified
     *
     * @see ITTSClient::initialize
     */
    virtual void uninitialize() = 0;

    /**
     * Registers a callback function to be called on TTS state changes
     *
     * @param [in] listener The callback function to be called when the state changes
     *
     * @note The listener might be called before this function returns
     *
     * @attention The listener might be called from the same thread as the caller of this method
     *            or from a different thread. The listener implementation is responsible for
     *            handling this scenario
     *
     * @see ITTSClient::unregisterListener
     */
    virtual void registerListener(StateChangeListener listener) = 0;

    /**
     * Unregisters a previously registered callback function, after which it won't be called
     * anymore to notify state changes
     *
     * @see ITTSClient::registerListener
     */
    virtual void unregisterListener() = 0;

    /**
     * Indicates whether TTS is enabled or disabled
     *
     * @retval true TTS is enabled
     * @retval false TTS is disabled
     */
    virtual bool isEnabled() = 0;

    /**
     * Destructor
     */
    virtual ~ITTSClient() {};
};