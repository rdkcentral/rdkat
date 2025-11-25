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

#include <atomic>
#include <iostream>

#include "ITTSClient.h"
#include "TTSClient.h"

/**
 * Thunder implementation of the ITTSClient interface
 *
 * The implementation provides the ability to transform text into audible speech using the
 * CTTSClientThunder::ITTSClient::speak method. It also provides a notification to a listener
 * registered using CTTSClientThunder::registerListener when TTS is enabled or disabled by the
 * user.
 */
class CTTSClientThunder : public ITTSClient, TTS::TTSConnectionCallback, TTS::TTSSessionCallback
{
public:
    /**
     * Constructs an instance of TTS Client Thunder implementation
     */
    CTTSClientThunder();

    /**
     * Destructor
     */
    ~CTTSClientThunder();

    // ITTSClient interface
    virtual void speak(const std::string &text);
    virtual bool initialize();
    virtual void uninitialize();
    virtual void registerListener(StateChangeListener listener);
    virtual void unregisterListener();
    virtual bool isEnabled();

private:
    // TTS interface

    /**
     * Callback handler to notify that a connection with the TTS server was established
     *
     * @see TTS component
     */
    virtual void onTTSServerConnected();

    /**
     * Callback handler to notify that a connection with the TTS server was closed
     *
     * @see TTS component
     */
    virtual void onTTSServerClosed();

    /**
     * Callback handler to notify that TTS state changed
     *
     * @see TTS component
     */
    virtual void onTTSStateChanged(bool enabled);

    /**
     * Callback handler to notify that a session was created with the TTS server
     *
     * @see TTS component
     */
    virtual void onTTSSessionCreated(uint32_t, uint32_t) {};

    /**
     * Callback handler to notify that speech has started
     *
     * @see TTS component
     */
    virtual void onSpeechStart(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data);

    /**
     * Callback handler to notify that speech has completed
     *
     * @see TTS component
     */
    virtual void onSpeechComplete(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data);

    /**
     * Callback handler to notify that an error has occured during speech attempt
     *
     * @see TTS component
     */
    virtual void onPlaybackError(uint32_t appId, uint32_t sessionId, uint32_t speechId);

    /**
     * Callback handler to notify of a network error
     *
     * @see TTS component
     */
    virtual void onNetworkError(uint32_t appId, uint32_t sessionId, uint32_t speechId);

private:
    /** Helper function to generate the application id required for the TTS session */
    static uint32_t generateAppId();

private:
    /**
     * Establish a session with the TTS server
     */
    void createSession();

    /**
     * Terminate a session with the TTS server
     */
    void destroySession();

private:
    /** Invalid TTS session ID */
    static inline constexpr uint32_t SESSION_ID_INVALID = 0;

    /** The next application id to use for the TTS session */
    static std::atomic_uint32_t next_app_id;

private:
    /** The application id for the current TTS session  */
    uint32_t m_app_id;

    /** The current TTS session ID */
    uint32_t m_session_id;

    /** Helper flag to track if the TTS Client is initialized  */
    bool m_initialized;

    /** Helper flag to track if TTS is enabled */
    bool m_tts_enabled;

    /** Helper flag to track if a TTS session shall be created */
    bool m_should_create_session;

    /** The TTS client (thunder) instance */
    TTS::TTSClient *m_tts_client;

    /** Counter with number of time text was spoken */
    uint32_t m_speech_counter;

    /** Listener for TTS state changes */
    StateChangeListener m_listener;
};