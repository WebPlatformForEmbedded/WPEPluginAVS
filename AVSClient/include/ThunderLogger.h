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
#pragma once

#include <mutex>
#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LogStringFormatter.h>

#include "Module.h"

namespace alexaClientSDK {
namespace sampleApp {

/**
 * A trace AVSSDK category for Thunder tracing
 */
class AVSSDK {
public:
    AVSSDK() = delete;
    AVSSDK(const AVSSDK& a_Copy) = delete;
    AVSSDK& operator=(const AVSSDK& a_RHS) = delete;

    explicit AVSSDK(const string& text);
    ~AVSSDK() = default;

    inline const char* Data() const;
    inline uint16_t Length() const;

private:
    std::string _text;
};

class AVSClient {
public:
    AVSClient() = delete;
    AVSClient(const AVSClient& a_Copy) = delete;
    AVSClient& operator=(const AVSClient& a_RHS) = delete;

    explicit AVSClient(const string& text);
    ~AVSClient() = default;

    inline const char* Data() const;
    inline uint16_t Length() const;

private:
    std::string _text;
};
/**
 * A simple class that logs/traces using the Thunder framework.
 */
class ThunderLogger : public avsCommon::utils::logger::Logger {
public:

    ThunderLogger(const ThunderLogger& a_Copy) = delete;
    ThunderLogger& operator=(const ThunderLogger& a_RHS) = delete;

    /**
     * Return the one and only @c ThunderLogger instance.
     *
     * @return The one and only @c ThunderLogger instance.
     */
    static std::shared_ptr<avsCommon::utils::logger::Logger> instance();

    /**
     * Trace a message through Thunder.
     *
     * @param stringToPrint The string to print.
     */
    static void trace(const std::string& stringToPrint);

    /**
     * Trace a message through Thunder with additional formatting.
     *
     * @param stringToPrint The string to print.
     */
    static void prettyTrace(const std::string& stringToPrint);

    /**
     * Trace a multilie message through Thunder with additional formatting.
     *
     * @param stringToPrint The string to print.
     */
    static void prettyTrace(std::initializer_list<std::string> lines);

    static void log(const std::string& stringToPrint);
    static void log(std::initializer_list<std::string> lines);

    void emit(
        avsCommon::utils::logger::Level level,
        std::chrono::system_clock::time_point time,
        const char* threadMoniker,
        const char* text) override;


private:
    /**
     * Constructor.
     */
    ThunderLogger();

    /**
     * Object used to format strings for log messages.
     */
    avsCommon::utils::logger::LogStringFormatter m_logFormatter;
};

}  // namespace sampleApp

namespace avsCommon {
namespace utils {
namespace logger {

/**
 * Return the singleton instance of @c ThunderLogger.
 *
 * @return The singleton instance of @c ThunderLogger.
 */
std::shared_ptr<avsCommon::utils::logger::Logger> getThunderLogger();

} // namespace logger
} // namespace utils
} // namespace avsCommon

}  // namespace alexaClientSDK
