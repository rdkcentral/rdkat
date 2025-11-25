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

#include "IScreenReader.h"
#include "IAtspiRegistryService.h"
#include "ITTSClient.h"

/**
 * Implementation of the IScreenReader interface
 *
 * The screen reader uses a AT-SPI2 registry service via the IAtspiRegistryService interface to
 * have access to relevant accessibility events and retrieve relevant accessibility related data
 * required to create the text that shall be send to a Text-To-Speech (TTS) service. The TTS
 * service is provided via the ITTSClient interface, allowing sending data to the TTS service
 * and query TTS state information.
 * The screen reader will listen for state changes from the TTS service to determine if it shall
 * listen for events from the registry service or not, depending whether TTS is enabled or
 * disabled.
 * Events received from the registry service will be processed to obtain text that shall be sent
 * to the TTS client, if applicable.
 * 
 * @see Check IScreenReader for constraints and assumptions related with thread safety
 */
class CScreenReader : public IScreenReader
{
public:
    /**
     * Constructs an instance of the screen reader
     *
     * @param [in] atspi_service The AT-SPI2 registry service to use to get accesibility related
     *                           events and data
     * @param [in] tts_client    The Text-To-Speech client to provide speech based on provided text
     */
    CScreenReader(IAtspiRegistryService *atspi_service, ITTSClient *tts_client);

    /**
     * Destructor
     */
    virtual ~CScreenReader();

    /**
     * Initializes the screen reader
     *
     * Once this method is called, the screen reader will be ready to process events from the
     * registry service and TTS client.
     */
    virtual void initialize();

    /**
     * Uninitialize the screen reader
     *
     * Once this method is called, the screen reader will not process any events from the registry
     * service or TTS client anymore.
     */
    virtual void uninitialize();

private:
    /**
     * Perform the screen reader initialization from the worker thread (not necessarily the same
     * as the caller context)
     */
    void initializeFromWorkerThread();

    /**
     * Perform the screen reader uninitialization from the worker thread (not necessarily the same
     * as the caller context)
     */
    void uninitializeFromWorkerThread();

    /**
     * Process the "Document:LoadComplete" event received from the registry service
     *
     * @param [in] event  The name of the event ("Document:LoadComplete")
     * @param [in] detail Not relevant for this event
     * @param [in] data1  Not relevant for this event
     * @param [in] data2  Not relevant for this event
     * @param [in] object An accessible object instance associated with the event to allow query of
     *                    additional data
     */
    void handleEventDocumentLoadComplete(const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object);

    /**
     * Process the "Object:StateChanged" event received from the registry service
     *
     * @param [in] event  The name of the event ("Object:StateChanged")
     * @param [in] detail Event specific description. E.g. "focused"
     * @param [in] data1  Event specific extra information. E.g "1" or "0" in case of a
     *                    "focused" detail which indicates whether the object is focused or not
     * @param [in] data2  Not relevant for this event
     * @param [in] object An accessible object instance associated with the event to allow query of
     *                    additional data
     */
    void handleEventObjectStateChanged(const std::string &event, const std::string &detail, uint32_t data1, uint32_t data2, IAtspiAccessibleObject &object);

    /**
     * Start listening for events from the registry service
     *
     * @see CScreenReader::stopAtspi2EventListeners
     */
    void startAtspi2EventListeners();

    /**
     * Stop listening for events from the registry service
     *
     * @see CScreenReader::startAtspi2EventListeners
     */
    void stopAtspi2EventListeners();

    /**
     * Callback function called from the worker thread indicating that indicates that TTS was
     * enabled
     *
     * This method will register event listeners with the registry service to be notified of
     * accessibility relevant events only when TTS is enabled.
     *
     * @see CScreenReader::ttsDisabledFromWorkerThread
     */
    void ttsEnabledFromWorkerThread();

    /**
     * Callback function called from the worker thread indicating that indicates that TTS was
     * disabled
     *
     * This method will unregister event listeners with the registry service to stop being notified
     * of accessibility relevant events when TTS is disabled.
     *
     * @see CScreenReader::ttsEnabledFromWorkerThread
     */
    void ttsDisabledFromWorkerThread();

    /**
     * Helper method to initializes internal states to allow synchronization between threads
     *
     * Ths supported synchronization is based on a thread A signaling a task has completed and a
     * thread B waiting for the task to complete
     *
     * @see CScreenReader::uninitSyncTask
     * @see CScreenReader::waitSyncTaskDone
     * @see CScreenReader::signalSyncTaskDone
     */
    void initSyncTask();

    /**
     * Helper method to uninitializes internal states and release resources, if applicable, used
     * for synchronization between threads
     *
     * @see CScreenReader::initSyncTask
     */
    void uninitSyncTask();

    /**
     * Helper method to wait for a task being completed, blocking current thread execution till
     * the task completion has been signaled
     *
     * @see CScreenReader::signalSyncTaskDone
     * @see CScreenReader::initSyncTask
     */
    void waitSyncTaskDone();

    /**
     * Helper method to signal a task as being completed, allowing another thread that is waiting
     * for the completion to unblock and continue processing.
     *
     * @see CScreenReader::waitSyncTaskDone
     * @see CScreenReader::initSyncTask
     */
    void signalSyncTaskDone();

private:
    /** Registry service used to receive accessibility related events and data */
    IAtspiRegistryService *m_atspi_service;

    /** TTS client to transform text into speech and provide state information about TTS service */
    ITTSClient *m_tts_client;

    /** Helper flag to track if the screen reader is initialized */
    bool m_initialized;

    /** Helper flag to track if the screen reader is enabled */
    bool m_enabled;

    /** Text that was sent to TTS service */
    std::string m_saved_text;

    /** Object for which text that was sent to TTS service */
    std::string m_saved_object_path;

    /** GLib worker thread context */
    GMainContext *m_worker_context;

    /** GLib worker thread loop */
    GMainLoop *m_worker_loop;

    /** GLib worker thread */
    GThread *m_worker_thread;

    /** Mutex to allow task synchronization. @see CScreenReader::signalSyncTaskDone */
    GMutex m_sync_task_mutex;

    /** Condition to allow task synchronization. @see CScreenReader::signalSyncTaskDone*/
    GCond m_sync_task_condition;

    /** Flag indicating if synchronous task has completed */
    bool m_sync_task_done;
};