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

# - Try to find  Yoga
# Once done this will define
#  YOGA_FOUND - System has Yoga
#  YOGA_INCLUDES - The Yoga include directories
#  YOGA_LIBRARIES - The libraries needed to use Yoga

find_path(YOGA_INCLUDES Yoga.h PATH_SUFFIXES yoga)
find_library(YOGA_LIBRARIES libyogacore.a)

find_package_handle_standard_args(YOGA DEFAULT_MSG
        YOGA_INCLUDES
        YOGA_LIBRARIES)
mark_as_advanced(YOGA_FOUND YOGA_INCLUDES YOGA_LIBRARIES)

if(YOGA_FOUND)
    if(NOT TARGET Yoga::Yoga)
        add_library(Yoga::Yoga SHARED IMPORTED)
        set_target_properties(Yoga::Yoga
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${YOGA_INCLUDES}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${YOGA_LIBRARIES}"
        )
    endif()
endif()
