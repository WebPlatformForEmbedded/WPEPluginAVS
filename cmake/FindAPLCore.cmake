# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

# - Try to find  APLCore
# Once done this will define
#  APL_CORE_FOUND - System has APLCore
#  APL_CORE_INCLUDES - The APLCore include directories
#  APL_CORE_LIBRARIES - The libraries needed to use APLCore

find_package(PkgConfig)
pkg_check_modules(PC_APL_CORE apl)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_APL_CORE DEFAULT_MSG PC_APL_CORE_FOUND)
mark_as_advanced(PC_APL_CORE_INCLUDE_DIRS PC_APL_CORE_LIBRARIES PC_APL_CORE_CFLAGS_OTHER)

if(PC_APL_CORE_FOUND)
    set(APL_CORE_FOUND ${PC_APL_CORE_FOUND})
    set(APL_CORE_INCLUDES ${PC_APL_CORE_INCLUDE_DIRS})
    set(APL_CORE_LIBRARIES ${PC_APL_CORE_LIBRARIES})
    # message(FATAL_ERROR "APL_CORE_LIBRARIES = ${APL_CORE_LIBRARIES}")
    if(NOT TARGET APLCore::APLCore)
        add_library(APLCore::APLCore SHARED IMPORTED)
        set_target_properties(APLCore::APLCore
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_APL_CORE_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_APL_CORE_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_APL_CORE_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_APL_CORE_LIBRARIES}")
    endif()
endif()