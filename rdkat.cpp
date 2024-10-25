/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
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

#include "rdkat.h"
#include "logger.h"

#include "TTSClient.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <atk/atk.h>

#include <sstream>

#define EVENT_OBJECT   "rdkat.Event.Object"
#define EVENT_WINDOW   "rdkat.Event.Window"
#define EVENT_DOCUMENT "rdkat.Event.Document"
#define EVENT_FOCUS    "rdkat.Event.Focus"

#define PROPERTY_CHANGE "PropertyChange"
#define STATE_CHANGED   "state-changed"

using namespace std;

namespace RDK_AT {

class RDKAt : public TTS::TTSConnectionCallback, public TTS::TTSSessionCallback {
    enum val_type {
        STRING,
        INT,
        POINTER
    };

public:
    static RDKAt& Instance() {
        static RDKAt rdk_at;
        return rdk_at;
    }

    ~RDKAt() {
        if(m_ttsClient) {
            delete m_ttsClient;
            m_ttsClient = NULL;
        }
        uninitialize();
    }

    void initialize(void);
    void enableProcessing(bool enable);
    void setVolumeControlCallback(MediaVolumeControlCallback cb, void *data);
    void uninitialize(void);

    void ensureTTSConnection();
    void createOrDestroySession();
    bool processingEnabled() { return m_process; }

    void resetMediaVolume() {
        if(RDKAt::Instance().m_mediaVolumeUpdated && RDKAt::Instance().m_mediaVolumeControlCB) {
            RDKAt::Instance().m_mediaVolumeControlCB(RDKAt::Instance().m_mediaVolumeControlCBData, 1);
            RDKAt::Instance().m_mediaVolumeUpdated = false;
        }
    }

    // TTS Session Callbacks
    virtual void onTTSServerConnected() {
        RDKLOG_INFO("Connection to TTSManager got established");
        // Handle TTSEngine crash & reconnection
        if(processingEnabled())
            m_shouldCreateSession = true;
    }

    virtual void onTTSServerClosed() {
        RDKLOG_ERROR("Connection to TTSManager got closed!!!");
        m_sessionId = 0;
        m_shouldCreateSession = false;
    }

    virtual void onTTSStateChanged(bool enabled) {
        m_ttsEnabled = enabled;
        RDKLOG_INFO("TTS is %s", m_ttsEnabled ? "enabled" : "disabled");
    }

    virtual void onTTSSessionCreated(uint32_t, uint32_t) {};
    virtual void onResourceAcquired(uint32_t, uint32_t) {};
    virtual void onResourceReleased(uint32_t, uint32_t) {};

    virtual void onSpeechStart(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data) {
        RDKLOG_INFO("appid=%d, sessionid=%d, speechid=%d, text=%s", appid, sessionid, data.id, data.text.c_str());
    }

    virtual void onNetworkError(uint32_t appId, uint32_t sessionId, uint32_t speechId) {
        RDKLOG_INFO("appid=%d, sessionid=%d, speechid=%d", appId, sessionId, speechId);
        resetMediaVolume();
    }

    virtual void onPlaybackError(uint32_t appId, uint32_t sessionId, uint32_t speechId) {
        RDKLOG_INFO("appid=%d, sessionid=%d, speechid=%d", appId, sessionId, speechId);
        resetMediaVolume();
    }

    virtual void onSpeechComplete(uint32_t appid, uint32_t sessionid, TTS::SpeechData &data) {
        RDKLOG_INFO("appid=%d, sessionid=%d, speechid=%d, text=%s", appid, sessionid, data.id, data.text.c_str());
        resetMediaVolume();
    }

    MediaVolumeControlCallback m_mediaVolumeControlCB;
    void *m_mediaVolumeControlCBData;

private:
    RDKAt() :
    m_mediaVolumeControlCB(NULL),
    m_mediaVolumeControlCBData(NULL),
    m_listenerIds(NULL),
    m_focusTrackerId(0),
    m_keyEventListenerId(0),
    m_initialized(false),
    m_process(false),
    m_ttsEnabled(false),
    m_mediaVolumeUpdated(false),
    m_appId(this),
    m_sessionId(0),
    m_shouldCreateSession(false),
    m_ttsClient(NULL),
    m_connectionAttempt(0) { }
    RDKAt(RDKAt &) {}

    inline static void printEventInfo(const std::string &klass, const std::string &major, const std::string &minor,
            guint32 d1, guint32 d2, const void *val, int type);
    inline static void printAccessibilityInfo(std::string &name, std::string &desc, std::string &role);
    static void HandleEvent(AtkObject *obj, std::string klass,
            const gchar* major_raw, const gchar* minor_raw, guint32 d1, guint32 d2, const void *val, int type);

    static gint KeyListener(AtkKeyEventStruct *event, gpointer data);
    static void FocusTracker(AtkObject *accObj);
    static gboolean PropertyEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean StateEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean WindowEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean DocumentEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean BoundsEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean ActiveDescendantEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean LinkSelectedEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean TextChangedEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean TextInsertEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean TextRemoveEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean ChildrenChangedEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);
    static gboolean GenericEventListener(GSignalInvocationHint *signal,
            guint param_count, const GValue *params, gpointer data);

    guint addSignalListener(GSignalEmissionHook listener, const char *signal_name);

    GArray *m_listenerIds;
    gint m_focusTrackerId;
    gint m_keyEventListenerId;

    bool m_initialized;
    bool m_process;
    bool m_ttsEnabled;
    bool m_mediaVolumeUpdated;
    uint32_t m_appId;
    uint32_t m_sessionId;
    bool m_shouldCreateSession;
    TTS::TTSClient *m_ttsClient;
    uint8_t m_connectionAttempt;
};

gint RDKAt::KeyListener(AtkKeyEventStruct *event, gpointer data)
{
    RDKLOG_TRACE("RDKAt::KeyListener()");
    return 0;
}

inline std::string checkNullAndReturnStr(char* temp){
    return (temp != NULL)? temp : std::string();
}

inline void getAccessibilityInfo(AtkObject *obj, std::string &name, std::string &desc, std::string &role) {
    name = checkNullAndReturnStr(atk_object_get_name(obj));
    desc = checkNullAndReturnStr(atk_object_get_description(obj));
    role = checkNullAndReturnStr(atk_role_get_name(atk_object_get_role(obj)));
}

#define PRINT_HELPER(ss, c, key, value) do {\
     if(!value.empty()) { \
         ss << (c ? ", " : "") << key << "=\"" << value << "\""; \
         c = true; \
     } \
 } while(0)

inline static void RDKAt::printEventInfo(const std::string &klass, const std::string &major, const std::string &minor,
        guint32 d1, guint32 d2, const void *val, int type) {

    if (!is_log_level_enabled(RDK_AT::VERBOSE_LEVEL))
        return;

    bool c = false;
    std::stringstream ss;

    PRINT_HELPER(ss, c, "class", klass);
    PRINT_HELPER(ss, c, "major", major);
    PRINT_HELPER(ss, c, "minor", minor);
    PRINT_HELPER(ss, c, "d1", std::to_string(d1));
    PRINT_HELPER(ss, c, "d2", std::to_string(d2));

    if(type == POINTER) {
        char buff[16] = {0};
        sprintf(buff, "%p", val);
        PRINT_HELPER(ss, c, "data", std::string(buff));
    } else if(type == INT) {
        PRINT_HELPER(ss, c, "data", std::to_string((int)val));
    } else if(type == STRING) {
        PRINT_HELPER(ss, c, "data", std::string((char *)val));
    }

    RDKLOG_VERBOSE("%s", ss.str().c_str());
}

inline void RDKAt::printAccessibilityInfo(std::string &name, std::string &desc, std::string &role) {
    if (!is_log_level_enabled(RDK_AT::VERBOSE_LEVEL))
        return;

    bool c = false;
    std::stringstream ss;

    PRINT_HELPER(ss, c, "name", name);
    PRINT_HELPER(ss, c, "desc", desc);
    PRINT_HELPER(ss, c, "role", role);

    RDKLOG_VERBOSE("%s", ss.str().c_str());
}

std::string getCellDescription(AtkObject *cell, AtkRole role) {
    static AtkObject *pCell = NULL;
    static AtkTable *pTableObj = NULL;
    static AtkObject *pRowObj = NULL;

    // On focusing a non-cell element, forget previous cell details
    if(role != ATK_ROLE_TABLE_CELL) {
        pCell = NULL;
        pTableObj = NULL;
        pRowObj = NULL;
        return string();
    }

    AtkObject *obj = cell;
    AtkTable *tableObj = NULL;
    AtkObject *rowObj = NULL;

    // Find Table Object
    while(obj) {
        obj = atk_object_get_parent(obj);
        if(!obj)
            break;

        role = atk_object_get_role(obj);
        if(role == ATK_ROLE_TABLE) {
            tableObj = ATK_TABLE(obj);
            break;
        } else if(role == ATK_ROLE_TABLE_ROW) {
            rowObj = obj;
        }
    }

    // Retrieve Table Caption
    std::string caption;
    if(tableObj) {
        AtkObject *captionObj = atk_table_get_caption(tableObj);
        if(captionObj && (cell == pCell || tableObj != pTableObj))
            caption = checkNullAndReturnStr(atk_object_get_name(captionObj));
    }

    // Retrieve Row Heading
    // Note : this is not the not Row Header element, but the accessible name of Row element
    std::string rowHeader;
    if(rowObj && (cell == pCell || rowObj != pRowObj)) {
       rowHeader = checkNullAndReturnStr(atk_object_get_name(rowObj));
    }

    // Remember Table information
    pTableObj = tableObj;
    pRowObj = rowObj;
    pCell = cell;

    std::string res;
    if(!caption.empty())
        res = caption + ". ";
    if(!rowHeader.empty())
        res += rowHeader + ". ";

    return res;
}

void RDKAt::ensureTTSConnection()
{
    if(!m_ttsClient) {
        if(m_connectionAttempt > 0)
            return;

        m_connectionAttempt++;
        m_ttsClient = TTS::TTSClient::create(this);
    }
}

void RDKAt::createOrDestroySession()
{
    if(!m_ttsClient)
        return;

    if(processingEnabled()) {
        if(m_sessionId == 0 && m_shouldCreateSession) {
            m_sessionId = m_ttsClient->createSession(m_appId, "WPE", this);
        }
    } else {
        if(m_sessionId != 0) {
            if(m_mediaVolumeControlCBData)
                m_mediaVolumeControlCB(m_mediaVolumeControlCBData, 1);

            m_ttsClient->abort(m_sessionId);
            m_ttsClient->destroySession(m_sessionId);
            m_sessionId = 0;
        }
    }
    m_shouldCreateSession = false;
}

void RDKAt::HandleEvent(AtkObject *obj, std::string klass,
        const gchar* major_raw, const gchar* minor_raw,
        guint32 d1, guint32 d2, const void *val, int type)
{
    static bool logProcessingError = true;
    if(!RDKAt::Instance().processingEnabled()) {
        if(logProcessingError)
            RDKLOG_ERROR("Processing ARIA Accessibility events are not enabled");
        logProcessingError = false;
        return;
    }
    logProcessingError = true;

    const std::string major = major_raw ? major_raw : "";
    const std::string minor = minor_raw ? minor_raw : "";

    printEventInfo(klass, major, minor, d1, d2, val, type);

    RDKAt::Instance().ensureTTSConnection();
    RDKAt::Instance().createOrDestroySession();

    // If TTS is not enabled, skip costly dom traversals as part of name & desc retrieval
    static bool enableDebugging = getenv("ENABLE_RDKAT_DEBUGGING");
    static bool logDebuggingDisabled = true;
    if(!RDKAt::Instance().m_ttsEnabled) {
        if(!enableDebugging) {
            if(logDebuggingDisabled)
                RDKLOG_ERROR("Both TTS & RDK-AT Debugging are disabled, not fetching accessibility info");
            logDebuggingDisabled = false;
            return;
        }
    }
    logDebuggingDisabled = true;

    TTS::SpeechData d;
    bool speak = false;
    std::string name, desc, role;
    static unsigned int counter = 0;
    if(major == "state-changed") {
        if(minor == "focused" && d1 == 1) {
            getAccessibilityInfo(obj, name, desc, role);
            printAccessibilityInfo(name, desc, role);

            d.text = name;
            if(!d.text.empty() && (role == "button" || role == "push button")) {
                d.text += " button";
            } else if(!d.text.empty() && (role == "check" || role == "check box")) {
                bool md = false;
                AtkStateSet *set = atk_object_ref_state_set(obj);
                md = set ? atk_state_set_contains_state(set, ATK_STATE_CHECKED) : false;
                d.text += (md ? " check box is checked" : " check box is unchecked");
            }

            if(!name.empty() && !desc.empty() && name != desc)
                d.text += (". " + desc);

            std::string cellDesc = getCellDescription(obj, atk_object_get_role(obj));
            if(!cellDesc.empty()) {
                RDKLOG_VERBOSE("Table Cell Description = \"%s\"", cellDesc.c_str());
                d.text = cellDesc + d.text;
            }

            speak = true;
        } else if(minor == "checked") {
            getAccessibilityInfo(obj, name, desc, role);
            printAccessibilityInfo(name, desc, role);

            d.text = name;
            if(!d.text.empty() && (role == "check" || role == "check box"))
                d.text += (d1 ? " check box is checked" : " check box is unchecked");
            speak = true;
        }
    } else if(major == "load-complete") {
        AtkRole atkrole = atk_object_get_role(obj);
        if(atkrole == ATK_ROLE_DOCUMENT_FRAME)
              return;
        getAccessibilityInfo(obj, name, desc, role);
        printAccessibilityInfo(name, desc, role);

        if(!name.empty()) {
            d.text = std::string(name) + " is loaded";
            speak = true;
        }
    }

    TTS::TTSClient *ttsClient = RDKAt::Instance().m_ttsClient;
    //it is temporary fix to Skip the duplication Text for YouTubeApp
    static std::string oldText;
    static AtkObject *oldObj = NULL;
    if(speak && !d.text.empty()) {
        if(d.text == oldText && obj == oldObj) {
            RDKLOG_VERBOSE("Skipping the duplication Text : \"%s\"", d.text.c_str());
        } else if(ttsClient) {
            if(ttsClient->isActiveSession(RDKAt::Instance().m_sessionId)) {
                if(!RDKAt::Instance().m_mediaVolumeUpdated && RDKAt::Instance().m_mediaVolumeControlCB) {
                    RDKAt::Instance().m_mediaVolumeControlCB(RDKAt::Instance().m_mediaVolumeControlCBData, 0.25);
                    RDKAt::Instance().m_mediaVolumeUpdated = true;
                }

                d.id = ++counter;
                ttsClient->speak(RDKAt::Instance().m_sessionId, d);
            } else {
                RDKLOG_WARNING("Session has not acquired resource to speak");
            }
        } else {
            RDKLOG_INFO("Text to Speak : \"%s\"", d.text.c_str());
        }
        oldText = d.text;
        oldObj = obj;
    }
}

void RDKAt::FocusTracker(AtkObject *accObj)
{
    RDKLOG_TRACE("RDKAt::FocusTracker()");
    HandleEvent(accObj, EVENT_FOCUS, "focus", "", 0, 0, 0, INT);
}

gboolean RDKAt::PropertyEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::PropertyEventListener()");

    gint i;
    const gchar *s1;
    AtkObject *tObj;
    AtkObject *accObj;
    const gchar *propName = NULL;
    AtkPropertyValues *propValues;

    accObj = (AtkObject*)g_value_get_object(&params[0]);
    propValues = (AtkPropertyValues *)g_value_get_pointer(&params[1]);
    propName = propValues[0].property_name;

    if(strcmp(propName, "accessible-name") == 0) {
        s1 = atk_object_get_name(accObj);
        if(s1 != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, s1, STRING);
    } else if(strcmp(propName, "accessible-description") == 0) {
        s1 = atk_object_get_description(accObj);
        if(s1 != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, s1, STRING);
    } else if(strcmp(propName, "accessible-parent") == 0) {
        tObj = atk_object_get_parent(accObj);
        if(tObj != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, tObj, POINTER);
    } else if(strcmp(propName, "accessible-role") == 0) {
        i = atk_object_get_role(accObj);
        HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, GINT_TO_POINTER(i), INT);
    } else if(strcmp(propName, "accessible-table-summary") == 0) {
        tObj = atk_table_get_summary(ATK_TABLE(accObj));
        if(tObj != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, tObj, POINTER);
    } else if(strcmp(propName, "accessible-table-column-header") == 0) {
        i = g_value_get_int(&(propValues->new_value));
        tObj = atk_table_get_column_header(ATK_TABLE(accObj), i);
        if(tObj != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, tObj, POINTER);
    } else if(strcmp(propName, "accessible-table-row-header") == 0) {
        i = g_value_get_int(&(propValues->new_value));
        tObj = atk_table_get_row_header(ATK_TABLE(accObj), i);
        if(tObj != NULL)
            HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, tObj, POINTER);
    } else if(strcmp(propName, "accessible-table-row-description") == 0) {
        i = g_value_get_int(&(propValues->new_value));
        s1 = atk_table_get_row_description(ATK_TABLE(accObj), i);
        HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, s1, STRING);
    } else if(strcmp(propName, "accessible-table-column-description") == 0) {
        i = g_value_get_int(&(propValues->new_value));
        s1 = atk_table_get_column_description(ATK_TABLE(accObj), i);
        HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, s1, STRING);
    } else if(strcmp(propName, "accessible-table-caption-object") == 0) {
        tObj = atk_table_get_caption(ATK_TABLE(accObj));
        HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, tObj, POINTER);
    } else {
        HandleEvent(accObj, EVENT_OBJECT, PROPERTY_CHANGE, propName, 0, 0, 0, INT);
    }

    return TRUE;
}

gboolean RDKAt::StateEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::StateEventListener()");

    AtkObject *accObj;
    const gchar *propName;
    guint d1;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    propName = g_value_get_string(&params[1]);

    d1 = (g_value_get_boolean(&params[2])) ? 1 : 0;
    HandleEvent(accObj, EVENT_OBJECT, STATE_CHANGED, propName, d1, 0, 0, INT);

    return TRUE;
}

gboolean RDKAt::WindowEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::WindowEventListener()");

    AtkObject *accObj;
    GSignalQuery signalQuery;
    const gchar *major, *str;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    str = atk_object_get_name(accObj);
    HandleEvent(accObj, EVENT_WINDOW, major, "", 0, 0, str, STRING);

    return TRUE;
}

gboolean RDKAt::DocumentEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::DocumentEventListener()");

    AtkObject *accObj;
    GSignalQuery signalQuery;
    const gchar *major, *str;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    str = atk_object_get_name(accObj);
    HandleEvent(accObj, EVENT_DOCUMENT, major, "", 0, 0, str, STRING);

    return TRUE;
}

gboolean RDKAt::BoundsEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::BoundsEventListener()");

    AtkObject *accObj;
    AtkRectangle *atk_rect;
    GSignalQuery signalQuery;
    const gchar *major;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));

    if(G_VALUE_HOLDS_BOXED(params + 1)) {
        atk_rect = (AtkRectangle*)g_value_get_boxed(params + 1);
        HandleEvent(accObj, EVENT_OBJECT, major, "", 0, 0, atk_rect, POINTER);
    }
    return TRUE;
}

gboolean RDKAt::ActiveDescendantEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::ActiveDescendantEventListener()");

    AtkObject *accObj;
    AtkObject *childObj;
    GSignalQuery signalQuery;
    const gchar *major;
    gint d1;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    childObj = ATK_OBJECT(g_value_get_pointer(&params[1]));
    g_return_val_if_fail(ATK_IS_OBJECT(childObj), TRUE);

    d1 = atk_object_get_index_in_parent(childObj);

    HandleEvent(accObj, EVENT_OBJECT, major, "", d1, 0, childObj, POINTER);
    return TRUE;
}

gboolean RDKAt::LinkSelectedEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::LinkSelectedEventListener()");

    AtkObject *accObj;
    GSignalQuery signalQuery;
    const gchar *major, *minor;
    gint d1 = 0;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    minor = g_quark_to_string(signal->detail);

    if(G_VALUE_TYPE(&params[1]) == G_TYPE_INT)
        d1 = g_value_get_int(&params[1]);

    HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, 0, 0, INT);
    return TRUE;
}

gboolean RDKAt::TextChangedEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::TextChangedEventListener()");

    AtkObject *accObj;
    GSignalQuery signalQuery;
    const gchar *major, *minor;
    gchar *selected;
    gint d1 = 0, d2 = 0;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    minor = g_quark_to_string(signal->detail);

    if(G_VALUE_TYPE(&params[1]) == G_TYPE_INT)
        d1 = g_value_get_int(&params[1]);

    if(G_VALUE_TYPE(&params[2]) == G_TYPE_INT)
        d2 = g_value_get_int(&params[2]);

    selected = atk_text_get_text(ATK_TEXT(accObj), d1, d1 + d2);

    HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, selected, STRING);
    g_free(selected);

    return TRUE;
}

gboolean RDKAt::TextInsertEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::TextInsertEventListener()");

    AtkObject *accObj;
    guint text_changed_signal_id;
    GSignalQuery signalQuery;
    const gchar *major = NULL;
    const gchar *minor_raw = NULL, *text = NULL;
    gchar *minor;
    gint d1 = 0, d2 = 0;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    text_changed_signal_id = g_signal_lookup("text-changed", G_OBJECT_TYPE(accObj));
    g_signal_query(text_changed_signal_id, &signalQuery);
    major = signalQuery.signal_name;

    minor_raw = g_quark_to_string(signal->detail);
    if(minor_raw)
        minor = g_strconcat("insert:", minor_raw, NULL);
    else
        minor = g_strdup("insert");

    if(G_VALUE_TYPE(&params[1]) == G_TYPE_INT)
        d1 = g_value_get_int(&params[1]);

    if(G_VALUE_TYPE(&params[2]) == G_TYPE_INT)
        d2 = g_value_get_int(&params[2]);

    if(G_VALUE_TYPE(&params[3]) == G_TYPE_STRING)
        text = g_value_get_string(&params[3]);

    HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, text, STRING);
    g_free(minor);
    return TRUE;
}

gboolean RDKAt::TextRemoveEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::TextRemoveEventListener()");

    AtkObject *accObj;
    guint text_changed_signal_id;
    GSignalQuery signalQuery;
    const gchar *major;
    const gchar *minor_raw = NULL, *text = NULL;
    gchar *minor;
    gint d1 = 0, d2 = 0;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    text_changed_signal_id = g_signal_lookup("text-changed", G_OBJECT_TYPE(accObj));
    g_signal_query(text_changed_signal_id, &signalQuery);
    major = signalQuery.signal_name;

    minor_raw = g_quark_to_string(signal->detail);

    if(minor_raw)
        minor = g_strconcat("delete:", minor_raw, NULL);
    else
        minor = g_strdup("delete");

    if(G_VALUE_TYPE(&params[1]) == G_TYPE_INT)
        d1 = g_value_get_int(&params[1]);

    if(G_VALUE_TYPE(&params[2]) == G_TYPE_INT)
        d2 = g_value_get_int(&params[2]);

    if(G_VALUE_TYPE(&params[3]) == G_TYPE_STRING)
        text = g_value_get_string(&params[3]);

    HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, text, STRING);
    g_free(minor);
    return TRUE;
}

gboolean RDKAt::ChildrenChangedEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::ChildrenChangedEventListener()");

    GSignalQuery signalQuery;
    const gchar *major, *minor;
    gint d1 = 0, d2 = 0;

    AtkObject *accObj, *tObj=NULL;
    gpointer pChild;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));
    minor = g_quark_to_string(signal->detail);

    d1 = g_value_get_uint(params + 1);
    pChild = g_value_get_pointer(params + 2);

    if(ATK_IS_OBJECT(pChild)) {
        tObj = ATK_OBJECT(pChild);
        HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, tObj, POINTER);
    } else if((minor != NULL) && (strcmp(minor, "add") == 0)) {
        tObj = atk_object_ref_accessible_child(accObj, d1);
        HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, tObj, POINTER);
        g_object_unref(tObj);
    } else {
        HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, tObj, POINTER);
    }

    return TRUE;
}

gboolean RDKAt::GenericEventListener(GSignalInvocationHint *signal,
        guint param_count, const GValue *params, gpointer data)
{
    RDKLOG_TRACE("RDKAt::GenericEventListener()");

    const gchar *major, *minor;
    AtkObject *accObj;
    GSignalQuery signalQuery;
    int d1 = 0, d2 = 0;

    g_signal_query(signal->signal_id, &signalQuery);
    major = signalQuery.signal_name;
    minor = g_quark_to_string(signal->detail);

    accObj = ATK_OBJECT(g_value_get_object(&params[0]));

    if(param_count > 1 && G_VALUE_TYPE(&params[1]) == G_TYPE_INT)
        d1 = g_value_get_int(&params[1]);

    if(param_count > 2 && G_VALUE_TYPE(&params[2]) == G_TYPE_INT)
        d2 = g_value_get_int(&params[2]);

    HandleEvent(accObj, EVENT_OBJECT, major, minor, d1, d2, 0, INT);

    return TRUE;
}

guint RDKAt::addSignalListener(GSignalEmissionHook listener, const char *signal_name)
{
    RDKLOG_TRACE("RDKAt::addSignalListener()");

    guint id = atk_add_global_event_listener(listener, signal_name);
    if(id > 0)
        g_array_append_val(m_listenerIds, id);

    return id;
}

void RDKAt::initialize()
{
    RDKLOG_TRACE("RDKAt::initialize()");
    if(m_initialized)
        return;
    m_initialized = true;

    GObject *gObject = (GObject*)g_object_new(ATK_TYPE_OBJECT, NULL);
    AtkObject *atkObject = atk_no_op_object_new(gObject);
    guint id = 0;

    g_object_unref(G_OBJECT(atkObject));
    g_object_unref(gObject);

    if(m_listenerIds) {
        g_warning("rdkat-register_event_listeners called multiple times");
        return;
    }

    m_listenerIds = g_array_sized_new(FALSE, TRUE, sizeof(guint), 16);
    m_focusTrackerId = atk_add_focus_tracker(FocusTracker);

    addSignalListener(PropertyEventListener, "Atk:AtkObject:property-change");

    id = addSignalListener(WindowEventListener, "window:create");
    if(id != 0) {
        addSignalListener(WindowEventListener, "window:destroy");
        addSignalListener(WindowEventListener, "window:minimize");
        addSignalListener(WindowEventListener, "window:maximize");
        addSignalListener(WindowEventListener, "window:restore");
        addSignalListener(WindowEventListener, "window:activate");
        addSignalListener(WindowEventListener, "window:deactivate");
    } else {
        addSignalListener(WindowEventListener, "Atk:AtkWindow:create");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:destroy");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:minimize");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:maximize");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:restore");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:activate");
        addSignalListener(WindowEventListener, "Atk:AtkWindow:deactivate");
    }

    addSignalListener(DocumentEventListener, "Atk:AtkDocument:load-complete");
    addSignalListener(DocumentEventListener, "Atk:AtkDocument:reload");
    addSignalListener(DocumentEventListener, "Atk:AtkDocument:load-stopped");

    addSignalListener(StateEventListener, "Atk:AtkObject:state-change");
    addSignalListener(GenericEventListener, "Atk:AtkObject:visible-data-changed");
    addSignalListener(ChildrenChangedEventListener, "Atk:AtkObject:children-changed");
    addSignalListener(ActiveDescendantEventListener, "Atk:AtkObject:active-descendant-changed");

    addSignalListener(GenericEventListener, "Atk:AtkTable:row-inserted");
    addSignalListener(GenericEventListener, "Atk:AtkTable:row-reordered");
    addSignalListener(GenericEventListener, "Atk:AtkTable:row-deleted");
    addSignalListener(GenericEventListener, "Atk:AtkTable:column-inserted");
    addSignalListener(GenericEventListener, "Atk:AtkTable:column-reordered");
    addSignalListener(GenericEventListener, "Atk:AtkTable:column-deleted");
    addSignalListener(GenericEventListener, "Atk:AtkTable:model-changed");
    addSignalListener(TextInsertEventListener, "Atk:AtkText:text-insert");
    addSignalListener(TextRemoveEventListener, "Atk:AtkText:text-remove");
    addSignalListener(TextChangedEventListener, "Atk:AtkText:text-changed");
    addSignalListener(GenericEventListener, "Atk:AtkText:text-caret-moved");
    addSignalListener(GenericEventListener, "Atk:AtkText:text-attributes-changed");
    addSignalListener(GenericEventListener, "Atk:AtkText:text-selection-changed");

    addSignalListener(BoundsEventListener, "Atk:AtkComponent:bounds-changed");
    addSignalListener(GenericEventListener, "Atk:AtkSelection:selection-changed");
    addSignalListener(LinkSelectedEventListener, "Atk:AtkHypertext:link-selected");

    m_keyEventListenerId = atk_add_key_event_listener(KeyListener, NULL);
}

void RDKAt::enableProcessing(bool enable)
{
    RDKLOG_INFO("processingEnabled=%d, enable=%d", processingEnabled(), enable);
    m_process = enable;
    m_shouldCreateSession = enable;

    createOrDestroySession();

    if(m_sessionId)
        m_ttsClient->abort(m_sessionId);
}

void RDKAt::setVolumeControlCallback(MediaVolumeControlCallback cb, void *data)
{
    RDKAt::Instance().m_mediaVolumeControlCB = cb;
    RDKAt::Instance().m_mediaVolumeControlCBData = data;
}

void RDKAt::uninitialize(void)
{
    RDKLOG_TRACE("RDKAt::uninitialize()");

    guint i;
    GArray *ids = m_listenerIds;
    m_listenerIds = NULL;

    if(m_focusTrackerId) {
        atk_remove_focus_tracker(m_focusTrackerId);
        m_focusTrackerId = 0;
    }

    if(ids) {
        for(i = 0; i < ids->len; i++) {
            atk_remove_global_event_listener(g_array_index(ids, guint, i));
        }
        g_array_free(ids, TRUE);
    }

    if(m_keyEventListenerId) {
        atk_remove_key_event_listener(m_keyEventListenerId);
        m_keyEventListenerId = 0;
    }
}

void Initialize()
{
    logger_init();

    RDKLOG_INFO("RDK_AT::Initialize()");
    RDKAt::Instance().initialize();
}

void EnableProcessing(bool enable)
{
    RDKLOG_INFO("RDK_AT::EnableProcessing()");
    RDKAt::Instance().enableProcessing(enable);
}

void SetVolumeControlCallback(MediaVolumeControlCallback cb, void *data)
{
    RDKLOG_INFO("RDK_AT::SetVolumeControlCallback()");
    RDKAt::Instance().setVolumeControlCallback(cb, data);
}

void Uninitialize()
{
    RDKLOG_TRACE("RDK_AT::Uninitialize()");
    RDKAt::Instance().uninitialize();
}

} // namespace RDK_AT
