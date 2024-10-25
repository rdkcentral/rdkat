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

#ifndef RDK_AT_H
#define RDK_AT_H

#include <iostream>
#include <string>
#include <list>

namespace RDK_AT {

// This callback is set from the caller to facilitate RDK_AT to control media volume on need.
// The callback takes the below params
// data - an opaque pointer data received from the caller on SetVolumeControlCallback()
// volume - volume level ranging [0-1], 0-no volume, 1-full media volume
typedef void (*MediaVolumeControlCallback)(void *data, float volume);

void Initialize();
void EnableProcessing(bool enable);
void SetVolumeControlCallback(MediaVolumeControlCallback cb, void *data);
void Uninitialize();

}

#endif // RDK_AT_H
