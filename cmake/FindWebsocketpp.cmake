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

# - Try to find  Websocketpp
# Once done this will define
#  WEBSOCKETPP_FOUND - System has Websocketpp
#  WEBSOCKETPP_INCLUDES - The Websocketpp include directories

find_path(WEBSOCKETPP_INCLUDES version.hpp PATH_SUFFIXES websocketpp)

find_package_handle_standard_args(WEBSOCKETPP DEFAULT_MSG
        WEBSOCKETPP_INCLUDES)
mark_as_advanced(WEBSOCKETPP_INCLUDES)

if(WEBSOCKETPP_FOUND)
    if(NOT TARGET Websocketpp::Websocketpp)
        add_library(Websocketpp::Websocketpp SHARED IMPORTED)
        set_target_properties(Websocketpp::Websocketpp
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${WEBSOCKETPP_INCLUDES}"
                # IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_WEBSOCKETPP_LIBRARIES}"
        )
    endif()
endif()
