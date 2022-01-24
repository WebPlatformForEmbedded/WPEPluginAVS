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

# - Try to find  AlexaClientSDK
# Once done this will define
#  ALEXA_CLIENT_SDK_FOUND - System has AlexaClientSDK
#  ALEXA_CLIENT_SDK_INCLUDES - The AlexaClientSDK include directories
#  ALEXA_CLIENT_SDK_LIBRARIES - The libraries needed to use AlexaClientSDK

find_package(PkgConfig)
pkg_check_modules(PC_ALEXA_CLIENT_SDK AlexaClientSDK)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_ALEXA_CLIENT_SDK DEFAULT_MSG PC_ALEXA_CLIENT_SDK_FOUND)

mark_as_advanced(PC_ALEXA_CLIENT_SDK_INCLUDE_DIRS PC_ALEXA_CLIENT_SDK_LIBRARIES PC_ALEXA_CLIENT_SDK_CFLAGS_OTHER)

if(PC_ALEXA_CLIENT_SDK_FOUND)
    set(ALEXA_CLIENT_SDK_FOUND ${PC_ALEXA_CLIENT_SDK_FOUND})
    set(ALEXA_CLIENT_SDK_INCLUDES ${PC_ALEXA_CLIENT_SDK_INCLUDE_DIRS})
    set(ALEXA_CLIENT_SDK_LIBRARIES ${PC_ALEXA_CLIENT_SDK_LIBRARIES})

    if(NOT TARGET AlexaClientSDK::AlexaClientSDK)
        add_library(AlexaClientSDK::AlexaClientSDK SHARED IMPORTED)
        set_target_properties(AlexaClientSDK::AlexaClientSDK
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_ALEXA_CLIENT_SDK_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_ALEXA_CLIENT_SDK_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_ALEXA_CLIENT_SDK_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_ALEXA_CLIENT_SDK_LIBRARIES}")
    endif()
endif()