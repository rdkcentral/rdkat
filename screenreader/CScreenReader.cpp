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

#include "logger.h"
#include "CScreenReader.h"

CScreenReader::CScreenReader(IAtspiRegistryService *atspi_service, ITTSClient *tts_client) : m_atspi_service(atspi_service),
                                                                                             m_tts_client(tts_client),
                                                                                             m_initialized(false),
                                                                                             m_enabled(false),
                                                                                             m_worker_context(nullptr),
                                                                                             m_worker_loop(nullptr),
                                                                                             m_worker_thread(nullptr),
                                                                                             m_sync_task_done(false)
{
    g_assert(m_atspi_service != nullptr);
    g_assert(m_tts_client != nullptr);
}

CScreenReader::~CScreenReader()
{
}

void CScreenReader::handleEventDocumentLoadComplete(const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object)
{
    std::string speech_data;
    bool speak = false;

    RDKLOG_TRACE("Handle Document LoadComplete event");

    RDKLOG_VERBOSE("Event handler: event = '%s' detail = '%s' data = '%u'", event.c_str(), detail.c_str(), data1);

    if (!m_enabled)
    {
        RDKLOG_WARNING("Screen Reader is not enabled. It may have been already deactivated.");
        return;
    }

    auto name = object.name();
    auto role = object.role();

    if (role == IAtspiAccessibleObject::ATSPI_ROLE_DOCUMENT_FRAME)
    {
        return;
    }
    if (!name.empty())
    {
        speech_data = name + " is loaded";
        speak = true;
    }
    if (speak)
    {
        m_tts_client->speak(speech_data);
    }
}

void CScreenReader::handleEventObjectStateChanged(const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object)
{
    std::string speech_data;
    bool speak = false;

    RDKLOG_TRACE("Handle Object StateChanged event");

    RDKLOG_VERBOSE("Event handler: event = '%s' detail = '%s' data = '%u'", event.c_str(), detail.c_str(), data1);

    if (!m_enabled)
    {
        RDKLOG_WARNING("Screen Reader is not enabled. It may have been already deactivated.");
        return;
    }

    if ((detail == "focused") && (data1 == 1))
    {
        auto name = object.name();
        auto description = object.description();
        auto role_name = object.roleName();

        speech_data = name;

        // TODO: Can we replace the role name check by checking the role?
        if (!speech_data.empty() && ((role_name == "button") || (role_name == "push button")))
        {
            speech_data += " button";
        }
        else if (!speech_data.empty() && ((role_name == "check") || (role_name == "check box")))
        {
            auto states = object.states();

            speech_data += (std::count(states.begin(), states.end(), IAtspiAccessibleObject::ATSPI_STATE_CHECKED) > 0)
                               ? " check box is checked"
                               : " check box is unchecked";
        }

        if (!name.empty() && !description.empty() && (name != description))
        {
            speech_data += ". " + description;
        }

        auto cell_description = object.cellDescription();
        if (!cell_description.empty())
        {
            speech_data = cell_description + speech_data;
        }

        speak = true;
    }
    else if (detail == "checked")
    {
        auto name = object.name();
        auto role_name = object.roleName();

        speech_data = name;

        if (!speech_data.empty() && ((role_name == "check") || (role_name == "check box")))
        {
            speech_data += (data1 ? " check box is checked" : " check box is unchecked");
        }
        speak = true;
    }

    if (!speak || speech_data.empty())
    {
        return;
    }

    if ((speech_data == m_saved_text) && (m_saved_object_path == object.objectPath()))
    {
        RDKLOG_VERBOSE("Skipping duplicate text: '%s'", speech_data.c_str());
    }
    else
    {
        RDKLOG_INFO("Speak text: '%s'", speech_data.c_str());

        m_tts_client->speak(speech_data);
    }
    m_saved_text = speech_data;
    m_saved_object_path = object.objectPath();
}

void CScreenReader::startAtspi2EventListeners()
{
    RDKLOG_TRACE("Start event listeners");

    g_assert(m_atspi_service != nullptr);

    if (m_atspi_service != nullptr)
    {
        m_atspi_service->registerEventListener("Document:LoadComplete", std::bind(&CScreenReader::handleEventDocumentLoadComplete, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        m_atspi_service->registerEventListener("Object:StateChanged", std::bind(&CScreenReader::handleEventObjectStateChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    }
    else
    {
        RDKLOG_WARNING("Registry Service is not running. I may have been shutdown already");
    }
}

void CScreenReader::stopAtspi2EventListeners()
{
    RDKLOG_TRACE("Stop event listeners");

    g_assert(m_atspi_service != nullptr);

    if (m_atspi_service != nullptr)
    {
        m_atspi_service->unregisterEventListener("Document:LoadComplete");
        m_atspi_service->unregisterEventListener("Object:StateChanged");
    }
    else
    {
        RDKLOG_WARNING("Registry Service is not running. I may have been shutdown already");
    }
}

void CScreenReader::ttsEnabledFromWorkerThread()
{
    RDKLOG_TRACE("Handling TTS enabled in worker thread");

    if (m_enabled)
    {
        return;
    }

    m_enabled = true;

    startAtspi2EventListeners();
}

void CScreenReader::ttsDisabledFromWorkerThread()
{
    RDKLOG_TRACE("Handling TTS disabled in worker thread");

    if (!m_enabled)
    {
        return;
    }

    m_enabled = false;

    stopAtspi2EventListeners();
}

void CScreenReader::initSyncTask()
{
    RDKLOG_TRACE("Initialize sync task");

    g_mutex_init(&m_sync_task_mutex);
    g_cond_init(&m_sync_task_condition);
    m_sync_task_done = false;
}

void CScreenReader::uninitSyncTask()
{
    RDKLOG_TRACE("Uninitialize sync task");

    g_cond_clear(&m_sync_task_condition);
    g_mutex_clear(&m_sync_task_mutex);
}

void CScreenReader::waitSyncTaskDone()
{
    RDKLOG_TRACE("Wait sync task done");

    g_mutex_lock(&m_sync_task_mutex);
    while (!m_sync_task_done)
    {
        g_cond_wait(&m_sync_task_condition, &m_sync_task_mutex);
    }
    m_sync_task_done = false;
    g_mutex_unlock(&m_sync_task_mutex);
}

void CScreenReader::signalSyncTaskDone()
{
    RDKLOG_TRACE("Signal sync task done");

    g_mutex_lock(&m_sync_task_mutex);
    m_sync_task_done = true;
    g_cond_signal(&m_sync_task_condition);
    g_mutex_unlock(&m_sync_task_mutex);
}

void CScreenReader::initializeFromWorkerThread()
{
    RDKLOG_TRACE("Initializing Screen Reader in worker thread");

    bool tts_ok = m_tts_client->initialize();
    if (!tts_ok)
    {
        RDKLOG_WARNING("Initializing Screen Reader in worker thread FAILED. TTS support might not be enabled");

        // Let invoker thread know we are done
        signalSyncTaskDone();
        return;
    }

    m_tts_client->registerListener([this](bool enabled) -> void
                                   {
                                       if (enabled)
                                       {
                                           // Ensure handling is done from correct context
                                           g_main_context_invoke(m_worker_context, [](gpointer user_data) -> gboolean
                                                                 {
                                                                     CScreenReader *screen_reader = static_cast<CScreenReader *>(user_data);
                                                                     screen_reader->ttsEnabledFromWorkerThread();
                                                                     return FALSE; // Single shot
                                                                 },
                                                                 this); //
                                       }
                                       else
                                       {
                                           // Ensure handling is done from correct context
                                           g_main_context_invoke(m_worker_context, [](gpointer user_data) -> gboolean
                                                                 {
                                                                     CScreenReader *screen_reader = static_cast<CScreenReader *>(user_data);

                                                                     screen_reader->ttsDisabledFromWorkerThread();
                                                                     return FALSE; // Single shot
                                                                 },
                                                                 this); //

                                       } });

    // Ensure atspi service listeners will be registered before starting the service to avoid race
    // condition with TTS listener invocation
    if (m_tts_client->isEnabled())
    {
        ttsEnabledFromWorkerThread();
    }

    m_atspi_service->start();

    m_initialized = true;

    // Let invoker thread know we are done
    signalSyncTaskDone();
}

void CScreenReader::uninitializeFromWorkerThread()
{
    RDKLOG_TRACE("Uninitializing Screen Reader in worker thread");

    if (!m_initialized)
    {
        return;
    }

    m_atspi_service->stop();

    m_tts_client->unregisterListener();
    m_tts_client->uninitialize();

    m_initialized = false;

    g_main_loop_quit(m_worker_loop);
}

void CScreenReader::initialize()
{
    RDKLOG_TRACE("Initializing Screen Reader");

    if (m_initialized)
    {
        RDKLOG_WARNING("Screen reader was already initialized.");
        return;
    }

    m_worker_context = g_main_context_new();
    g_assert(m_worker_context != nullptr);

    m_worker_loop = g_main_loop_new(m_worker_context, FALSE);
    g_assert(m_worker_loop != nullptr);

    initSyncTask();

    // Start worker thread that will handle the initialization and shutdown tasks and run
    // the main loop for the service
    m_worker_thread = g_thread_new("ScreenReader", [](gpointer data) -> gpointer
                                   {
                                       CScreenReader *screen_reader = static_cast<CScreenReader *>(data);

                                       g_main_context_push_thread_default(screen_reader->m_worker_context);

                                       g_main_loop_run(screen_reader->m_worker_loop);

                                       g_main_context_pop_thread_default(screen_reader->m_worker_context);

                                       return NULL; // Nothing to return
                                   },
                                   this);

    // Post initialization task to run in worker thread
    g_main_context_invoke(m_worker_context, [](gpointer user_data) -> gboolean
                          {
                              CScreenReader *screen_reader = static_cast<CScreenReader *>(user_data);

                              screen_reader->initializeFromWorkerThread();

                              return FALSE; // Single shot
                          },
                          this);

    // Wait initialization task to complete
    waitSyncTaskDone();

    // Clean up what's not needed anymore
    uninitSyncTask();
}

void CScreenReader::uninitialize()
{
    RDKLOG_TRACE("Uninitializing Screen Reader");

    if (!m_initialized)
    {
        RDKLOG_WARNING("Screen reader was not initialized before.");
        return;
    }
    g_main_context_invoke(m_worker_context, [](gpointer user_data) -> gboolean
                          {
                              CScreenReader *screen_reader = static_cast<CScreenReader *>(user_data);

                              screen_reader->uninitializeFromWorkerThread();

                              return FALSE; // Single shot
                          },
                          this);

    // Wait till the shutdown has completed
    g_thread_join(m_worker_thread);

    g_main_loop_unref(m_worker_loop);
    g_main_context_unref(m_worker_context);

    m_worker_thread = nullptr;
    m_worker_loop = nullptr;
    m_worker_context = nullptr;
}
