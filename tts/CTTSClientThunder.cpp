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

#include "CTTSClientThunder.h"
#include "logger.h"

std::atomic_uint32_t CTTSClientThunder::next_app_id = 1;

CTTSClientThunder::CTTSClientThunder()
    : m_app_id(CTTSClientThunder::generateAppId()),
      m_session_id(SESSION_ID_INVALID),
      m_initialized(false),
      m_tts_enabled(false),
      m_should_create_session(false),
      m_tts_client(NULL),
      m_speech_counter(0),
      m_listener(nullptr)
{
}

CTTSClientThunder::~CTTSClientThunder()
{
    uninitialize();
}

void CTTSClientThunder::speak(const std::string &text)
{
    RDKLOG_TRACE("Speaking text");

    if (!m_initialized)
    {
        RDKLOG_WARNING("TTS Client not iniitalized, so unable to speak");
        return;
    }

    // Ensure we have an active session in case a previous session has closed
    createSession();

    if (m_tts_client->isActiveSession(m_session_id))
    {
        TTS::SpeechData data;

        // TODO if(!RDKAt::Instance().m_mediaVolumeUpdated && RDKAt::Instance().m_mediaVolumeControlCB) {
        //     RDKAt::Instance().m_mediaVolumeControlCB(RDKAt::Instance().m_mediaVolumeControlCBData, 0.25);
        //     RDKAt::Instance().m_mediaVolumeUpdated = true;
        // }

        RDKLOG_VERBOSE("Speaking text: '%s'", text.c_str());

        data.text = text;
        data.id = ++m_speech_counter;

        m_tts_client->speak(m_session_id, data);
    }
    else
    {
        RDKLOG_WARNING("Session has not acquired resource to speak");
    }
}

bool CTTSClientThunder::isEnabled()
{
    return m_tts_enabled;
}

bool CTTSClientThunder::initialize()
{
    RDKLOG_TRACE("Initializing TTS Client");

    if (m_initialized)
    {
        return m_initialized;
    }
    m_tts_client = TTS::TTSClient::create(this);
    if (!m_tts_client)
    {
        RDKLOG_ERROR("Unable to create TTS Client instance");
        return false;
    }
    m_initialized = true;

    m_tts_enabled = m_tts_client->isTTSEnabled();

    m_should_create_session = true;

    createSession();

    return true;
}

void CTTSClientThunder::uninitialize()
{
    RDKLOG_TRACE("Uninitializing TTS Client");

    if (!m_initialized)
    {
        return;
    }
    m_initialized = false;
    destroySession();

    delete m_tts_client;
    m_tts_client = NULL;
}

void CTTSClientThunder::registerListener(StateChangeListener listener)
{
    RDKLOG_TRACE("Register state change listener");

    g_assert(m_initialized);
    g_assert(listener);

    m_listener = listener;

    // Notify current state
    m_listener(m_tts_enabled);
}

void CTTSClientThunder::unregisterListener()
{
    RDKLOG_TRACE("Unregister state change listener");

    g_assert(m_initialized);

    m_listener = nullptr;
}

void CTTSClientThunder::createSession()
{
    if (m_session_id != SESSION_ID_INVALID || !m_should_create_session)
    {
        return;
    }
    m_should_create_session = false;

    m_session_id = m_tts_client->createSession(m_app_id, "WPE", this);
    if (m_session_id == SESSION_ID_INVALID)
    {
        RDKLOG_ERROR("Unable to create TTS session!");
        return;
    }
    RDKLOG_VERBOSE("Created TTS client session with id '%u'", m_session_id);
}

void CTTSClientThunder::destroySession()
{
    if (m_session_id == SESSION_ID_INVALID)
    {
        return;
    }
    // TODO if(m_mediaVolumeControlCBData)
    // TODO    m_mediaVolumeControlCB(m_mediaVolumeControlCBData, 1);
    m_tts_client->abort(m_session_id);
    m_tts_client->destroySession(m_session_id);

    RDKLOG_VERBOSE("Destroyed TTS client session with id '%u'", m_session_id);

    m_session_id = SESSION_ID_INVALID;
}

void CTTSClientThunder::onTTSServerConnected()
{
    RDKLOG_INFO("Connection to TTS server got established");

    // Handle TTSEngine crash & reconnection
    if (m_initialized)
    {
        // TODO: Not thread safe. Ensure these are changed from same thread
        m_should_create_session = true;
    }
}

void CTTSClientThunder::onTTSServerClosed()
{
    RDKLOG_WARNING("Connection to TTS server got closed");

    // TODO: Not thread safe. Ensure these are changed from same thread
    m_session_id = 0;
    m_should_create_session = false;
}

void CTTSClientThunder::onTTSStateChanged(bool enabled)
{
    RDKLOG_TRACE("TTS state changed");

    m_tts_enabled = enabled;

    RDKLOG_INFO("TTS state changed. TTS is %s", m_tts_enabled ? "enabled" : "disabled");

    if (m_listener)
    {
        m_listener(enabled);
    }
    else
    {
        RDKLOG_WARNING("No listener registered");
    }
}

void CTTSClientThunder::onSpeechStart(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data)
{
    RDKLOG_VERBOSE("Speech starting: appid=%d, sessionid=%d, speechid=%d, text=%s", appid, sessionid, data.id, data.text.c_str());
}

void CTTSClientThunder::onSpeechComplete(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data)
{
    RDKLOG_VERBOSE("Speech complete: appid=%d, sessionid=%d, speechid=%d, text=%s", appid, sessionid, data.id, data.text.c_str());

    // TODO    resetMediaVolume();
}

void CTTSClientThunder::onPlaybackError(uint32_t appId, uint32_t sessionId, uint32_t speechId)
{
    RDKLOG_WARNING("Playback error: appid=%d, sessionid=%d, speechid=%d", appId, sessionId, speechId);

    // TODO    resetMediaVolume();
}

void CTTSClientThunder::onNetworkError(uint32_t appId, uint32_t sessionId, uint32_t speechId)
{
    RDKLOG_WARNING("Network error: appid=%d, sessionid=%d, speechid=%d", appId, sessionId, speechId);

    // TODO    resetMediaVolume();
}

uint32_t CTTSClientThunder::generateAppId()
{
    return next_app_id++;
}
