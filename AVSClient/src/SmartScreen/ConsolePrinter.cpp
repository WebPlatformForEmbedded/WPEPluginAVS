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

#include "ConsolePrinter.h"

#include <iostream>
#include <algorithm>

/**
 *  When using pretty print, we pad our strings in the beginning and in the end with the margin representation '#'
 *  and 7 spaces. E.g., if I pass "Hello world!" string, pretty print will look like:
 *  ############################
 *  #       Hello world!       #
 *  ############################
 */
static const size_t PADDING_LENGTH = 8;

namespace alexaSmartScreenSDK {
namespace sampleApp {

using namespace alexaClientSDK;

std::shared_ptr<std::mutex> ConsolePrinter::m_globalMutex = std::make_shared<std::mutex>();

ConsolePrinter::ConsolePrinter() :
        avsCommon::utils::logger::Logger(avsCommon::utils::logger::Level::UNKNOWN),
        m_mutex(m_globalMutex) {
}

void ConsolePrinter::simplePrint(const std::string& stringToPrint) {
    auto mutex = m_globalMutex;
    if (!mutex) {
        return;
    }

    std::lock_guard<std::mutex> lock{*mutex};
    std::cout << stringToPrint << std::endl;
}

void ConsolePrinter::prettyPrint(std::initializer_list<std::string> lines) {
    size_t maxLength = 0;
    for (auto& line : lines) {
        maxLength = std::max(line.size(), maxLength);
    }

    const std::string line(maxLength + (2 * PADDING_LENGTH), '#');
    std::ostringstream oss;
    oss << line << std::endl;

    // Write each line starting and ending with '#'
    auto padBegin = std::string("#");
    padBegin.append(PADDING_LENGTH - 1, ' ');
    for (auto& line : lines) {
        auto padEnd = std::string("#");
        padEnd.insert(padEnd.begin(), maxLength - line.size() + (PADDING_LENGTH - 1), ' ');
        oss << padBegin << line << padEnd << std::endl;
    }

    oss << line << std::endl;
    simplePrint(oss.str());
}

void ConsolePrinter::prettyPrint(const std::string& stringToPrint) {
    prettyPrint({stringToPrint});
}

void ConsolePrinter::emit(
    avsCommon::utils::logger::Level level,
    std::chrono::system_clock::time_point time,
    const char* threadMoniker,
    const char* text) {
    std::lock_guard<std::mutex> lock{*m_mutex};
    std::cout << m_logFormatter.format(level, time, threadMoniker, text) << std::endl;
}

}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK
