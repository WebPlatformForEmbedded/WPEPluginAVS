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
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AplCoreEngineSDKLogBridge.h"

#include <AVSCommon/Utils/Logger/Logger.h>

static const std::string TAG("AplCoreEngine");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaSmartScreenSDK {
namespace sampleApp {

void AplCoreEngineSDKLogBridge::transport(apl::LogLevel level, const std::string& log) {
    switch (level) {
        case apl::LogLevel::TRACE:
            ACSDK_DEBUG9(LX("Log").m(log));
            break;
            // TODO: Same problem as in AplCoreGuiRenderer.h but not solved by undef by some reason.
        case static_cast<apl::LogLevel>(1):
            ACSDK_DEBUG3(LX("Log").m(log));
            break;
        case apl::LogLevel::INFO:
            ACSDK_INFO(LX("Log").m(log));
            break;
        case apl::LogLevel::WARN:
            ACSDK_WARN(LX("Log").m(log));
            break;
        case apl::LogLevel::ERROR:
            ACSDK_ERROR(LX("Log").m(log));
            break;
        case apl::LogLevel::CRITICAL:
            ACSDK_CRITICAL(LX("Log").m(log));
            break;
        default:
            ACSDK_ERROR(LX("unknownLevelLog").d("level", static_cast<int>(level)).m(log));
            return;
    }
}
}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK