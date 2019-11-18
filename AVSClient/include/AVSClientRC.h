/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <cstdlib>

namespace alexaClientSDK {
namespace sampleApp {

enum AVSClientRC {
    /// The AVSClient exits in success.
    OK = EXIT_SUCCESS,
    /// The AVSClient exits in failure.
    ERROR = EXIT_FAILURE,
    /// The AVSClient exits and needs a restart.
    RESTART,
    /// The AVSClient is busy and requested action is not done
    BUSY,
    /// The action is not implemented/unavailable
    UNAVAILABLE
};

}  // namespace sampleApp
}  // namespace alexaClientSDK
