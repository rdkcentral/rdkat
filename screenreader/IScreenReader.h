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

/**
 * Interface for Screen Reader Implementation
 *
 * An implementation of this interface shall provide the required functionality to listen
 * for accessibility related events, fetch relevant accessibility related data and
 * transform it into relevant text that shall be processed by a Text-To-Speech (TTS)
 * service.
 *
 * The screen reader shall start processing any accessibility related data once the
 * IScreenReader::initialize method has been called, sending the text to a TTS service when
 * applicable. Once IScreenReader::uninitialize is called, any processing shall be stopped
 * and resources released.
 *
 * This processing depends on the TTS service being enabled or not, which the screen reader
 * implementation will check to enable / disable processing as needed.
 *
 * @attention Public methods shall be called from the same thread context to avoid thread safety
 *            issues
 */
class IScreenReader
{
public:
    /**
     * Initializes the screen reader to start processing accessibility related events and send
     * resulting text to a TTS service
     *
     * The processing is conditioned by the TTS service being enabled or disabled, but the
     * implementation shall observe any changes in the TTS state to enable / disable processing
     * as needed.
     *
     * @see IScreenReader::uninitialize
     */
    virtual void initialize() = 0;

    /**
     * Unitializes the screen reader to stop processing accessibility related events
     *
     * @see IScreenReader::initialize
     */
    virtual void uninitialize() = 0;

    /**
     * Destructor
     */
    virtual ~IScreenReader() {}
};