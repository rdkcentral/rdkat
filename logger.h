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
#ifndef RDK_AT_LOGGER_H
#define RDK_AT_LOGGER_H

namespace RDK_AT
{

/**
 * Logging level with an increasing order of refinement
 * (TRACE_LEVEL = Finest logging).
 * It is essental to start with 0 and increase w/o gaps as the value
 * can be used for indexing in a mapping table.
 */
enum LogLevel {FATAL_LEVEL = 0, ERROR_LEVEL, WARNING_LEVEL, INFO_LEVEL, VERBOSE_LEVEL, TRACE_LEVEL};

/**
 * @brief Init logging
 * Should be called once per program run before calling log-functions
 */
void logger_init();

/**
 * @brief Checks if logging is enabled for log level
 */
bool is_log_level_enabled(LogLevel level);

/**
 * @brief Log a message
 * The function is defined by logging backend.
 * Currently 2 variants are supported: rdk_logger (USE_RDK_LOGGER),
 *                                     stdout(default)
 */
void log(LogLevel level,
    const char* func,
    const char* file,
    int line,
    int threadID,
    const char* format, ...);

#define _LOG(LEVEL, FORMAT, ...)          \
    RDK_AT::log(LEVEL,                       \
         __func__, __FILE__, __LINE__, 0, \
         FORMAT,                          \
         ##__VA_ARGS__)

#define RDKLOG_TRACE(FMT, ...)   _LOG(RDK_AT::TRACE_LEVEL, FMT, ##__VA_ARGS__)
#define RDKLOG_VERBOSE(FMT, ...) _LOG(RDK_AT::VERBOSE_LEVEL, FMT, ##__VA_ARGS__)
#define RDKLOG_INFO(FMT, ...)    _LOG(RDK_AT::INFO_LEVEL, FMT, ##__VA_ARGS__)
#define RDKLOG_WARNING(FMT, ...) _LOG(RDK_AT::WARNING_LEVEL, FMT, ##__VA_ARGS__)
#define RDKLOG_ERROR(FMT, ...)   _LOG(RDK_AT::ERROR_LEVEL, FMT, ##__VA_ARGS__)
#define RDKLOG_FATAL(FMT, ...)   _LOG(RDK_AT::FATAL_LEVEL, FMT, ##__VA_ARGS__)

} // namespace RDK_AT

#endif  // RDK_AT_LOGGER_H
