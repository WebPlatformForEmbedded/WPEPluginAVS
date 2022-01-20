# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# - Try to find  AlexaSmartScreenSDK
# Once done this will define
#  ALEXA_SMART_SCREEN_SDK_FOUND - System has AlexaSmartScreenSDK
#  ALEXA_SMART_SCREEN_SDK_INCLUDES - The AlexaSmartScreenSDK include directories
#  ALEXA_SMART_SCREEN_SDK_LIBRARIES - The libraries needed to use AlexaSmartScreenSDK

find_package(PkgConfig)
pkg_check_modules(PC_ALEXA_SMART_SCREEN_SDK AlexaSmartScreenSDK)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_ALEXA_SMART_SCREEN_SDK DEFAULT_MSG PC_ALEXA_SMART_SCREEN_SDK_FOUND)
mark_as_advanced(PC_ALEXA_SMART_SCREEN_SDK_INCLUDE_DIRS PC_ALEXA_SMART_SCREEN_SDK_LIBRARIES PC_ALEXA_SMART_SCREEN_SDK_CFLAGS_OTHER)

if(PC_ALEXA_SMART_SCREEN_SDK_FOUND)
    set(ALEXA_SMART_SCREEN_SDK_FOUND ${PC_ALEXA_SMART_SCREEN_SDK_FOUND})
    set(ALEXA_SMART_SCREEN_SDK_INCLUDES ${PC_ALEXA_SMART_SCREEN_SDK_INCLUDE_DIRS})
    set(ALEXA_SMART_SCREEN_SDK_LIBRARIES ${PC_ALEXA_SMART_SCREEN_SDK_LIBRARIES})

    if(NOT TARGET AlexaSmartScreenSDK::AlexaSmartScreenSDK)
        add_library(AlexaSmartScreenSDK::AlexaSmartScreenSDK SHARED IMPORTED)
        set_target_properties(AlexaSmartScreenSDK::AlexaSmartScreenSDK
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_ALEXA_SMART_SCREEN_SDK_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_ALEXA_SMART_SCREEN_SDK_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_ALEXA_SMART_SCREEN_SDK_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_ALEXA_SMART_SCREEN_SDK_LIBRARIES}")
    endif()
endif()