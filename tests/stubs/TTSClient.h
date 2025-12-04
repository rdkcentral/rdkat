#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

namespace TTS {

enum TTS_Error {
    TTS_OK = 0,
    TTS_FAIL,
    TTS_NOT_ENABLED,
    TTS_CREATE_SESSION_DUPLICATE,
    TTS_EMPTY_APPID_INPUT,
    TTS_RESOURCE_BUSY,
    TTS_NO_SESSION_FOUND,
    TTS_NESTED_CLAIM_REQUEST,
    TTS_INVALID_CONFIGURATION,
    TTS_SESSION_NOT_ACTIVE,
    TTS_APP_NOT_FOUND,
    TTS_POLICY_VIOLATION,
    TTS_OBJECT_DESTROYED = 1010,
    TTS_SPEECH_NOT_FOUND,
};

struct SpeechData {
    SpeechData() : secure(true), id(0) {}
    SpeechData(uint32_t i) : secure(true), id(i) {}
    ~SpeechData() {}

    bool secure;
    uint32_t id;
    std::string text;
};

class TTSConnectionCallback {
public:
    TTSConnectionCallback() {}
    virtual ~TTSConnectionCallback() {}

    virtual void onTTSServerConnected() {}
    virtual void onTTSServerClosed() {}
    virtual void onTTSStateChanged(bool enabled) { (void)enabled; }
    virtual void onVoiceChanged(std::string voice) { (void)voice; }
};

class TTSSessionCallback {
public:
    TTSSessionCallback() {}
    virtual ~TTSSessionCallback() {}

    virtual void onTTSSessionCreated(uint32_t appId, uint32_t sessionId) { (void)appId; (void)sessionId; }
    virtual void onResourceAcquired(uint32_t appId, uint32_t sessionId) { (void)appId; (void)sessionId; }
    virtual void onResourceReleased(uint32_t appId, uint32_t sessionId) { (void)appId; (void)sessionId; }
    virtual void onWillSpeak(uint32_t appId, uint32_t sessionId, SpeechData &data) { (void)appId; (void)sessionId; (void)data; }
    virtual void onSpeechStart(uint32_t appId, uint32_t sessionId, SpeechData &data) { (void)appId; (void)sessionId; (void)data; }
    virtual void onSpeechPause(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onSpeechResume(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onSpeechCancelled(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onSpeechInterrupted(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onNetworkError(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onPlaybackError(uint32_t appId, uint32_t sessionId, uint32_t speechId) { (void)appId; (void)sessionId; (void)speechId; }
    virtual void onSpeechComplete(uint32_t appId, uint32_t sessionId, SpeechData &data) { (void)appId; (void)sessionId; (void)data; }
};

class TTSClient {
public:
    static TTSClient *create(TTSConnectionCallback *connCallback, bool discardRtDispatching=false) 
    {
        return new TTSClient(connCallback, discardRtDispatching);
    }
    virtual ~TTSClient() {}

    bool isTTSEnabled(bool forcefetch=false) {return false;}

    uint32_t createSession(uint32_t appid, std::string appname, TTSSessionCallback *sessCallback) { return 1; }
    TTS_Error destroySession(uint32_t sessionid) { return TTS_OK; };
    bool isActiveSession(uint32_t sessionid, bool forcefetch=false) { return true; };


    TTS_Error speak(uint32_t sessionid, SpeechData& data) { return TTS_OK; };
    TTS_Error pause(uint32_t sessionid, uint32_t speechid) { return TTS_OK; };
    TTS_Error resume(uint32_t sessionid, uint32_t speechid) { return TTS_OK; };
    TTS_Error abort(uint32_t sessionid, bool clearPending = false) { return TTS_OK; };


private:
    TTSClient(TTSConnectionCallback *client, bool discardRtDispatching=false) {}
    TTSClient(TTSClient&) = delete;
};

} // namespace TTS
