/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "ThunderLogger.h"

#include <WPEFramework/core/Trace.h>
#include <WPEFramework/tracing/tracing.h>

// using namespace WPEFramework;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::logger;

namespace alexaClientSDK {
namespace sampleApp {

/// Configuration key for DefaultLogger settings
static const std::string CONFIG_KEY_DEFAULT_LOGGER = "thunderLogger";

AVSSDK::AVSSDK(const string& text)
    : _text(WPEFramework::Core::ToString(text))
{
}

inline const char* AVSSDK::Data() const
{
    return (_text.c_str());
}

inline uint16_t AVSSDK::Length() const
{
    return (static_cast<uint16_t>(_text.length()));
}

AVSClient::AVSClient(const string& text)
    : _text(WPEFramework::Core::ToString(text))
{
}

inline const char* AVSClient::Data() const
{
    return (_text.c_str());
}

inline uint16_t AVSClient::Length() const
{
    return (static_cast<uint16_t>(_text.length()));
}

std::shared_ptr<Logger> ThunderLogger::instance() {
    static std::shared_ptr<Logger> singleThunderLogger = std::shared_ptr<ThunderLogger>(new ThunderLogger);
    return singleThunderLogger;
}

ThunderLogger::ThunderLogger() :
        Logger(Level::UNKNOWN) {
   init(configuration::ConfigurationNode::getRoot()[CONFIG_KEY_DEFAULT_LOGGER]);
}

void ThunderLogger::trace(const std::string& stringToPrint) {
    TRACE_L1("AVSClient - %s", stringToPrint.c_str());
}

void ThunderLogger::prettyTrace(const std::string& stringToPrint) {
    trace(stringToPrint);
}

void ThunderLogger::prettyTrace(std::initializer_list<std::string> lines) {
    for(const auto& line : lines){
        prettyTrace(line);
    }
}

void ThunderLogger::log(const std::string& stringToPrint) {
    TRACE_GLOBAL(AVSClient, (stringToPrint.c_str()));
}

void ThunderLogger::log(std::initializer_list<std::string> lines) {
    for(const auto& line : lines){
        log(line);
    }
}

void ThunderLogger::emit(
    avsCommon::utils::logger::Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {

    std::stringstream ss;
    ss << "[" << threadMoniker << "] " << convertLevelToChar(level) << " " << text;
    TRACE(AVSSDK, (ss.str().c_str()));
}
}  // namespace sampleApp

namespace avsCommon {
namespace utils {
namespace logger {

std::shared_ptr<Logger> getThunderLogger() {
    return alexaClientSDK::sampleApp::ThunderLogger::instance();
}
} // namespace logger
} // namespace utils
} // namespace avsCommon

} // namespace alexaClientSDK
