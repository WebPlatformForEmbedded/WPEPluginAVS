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

# - Try to find  Portaudio
# Once done this will define
#  PORTAUDIO_FOUND - System has Portaudio
#  PORTAUDIO_INCLUDES - The Portaudio include directories
#  PORTAUDIO_LIBRARIES - The libraries needed to use Portaudio

find_package(PkgConfig)
pkg_check_modules(PC_PORTAUDIO REQUIRED portaudio-2.0)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_PORTAUDIO DEFAULT_MSG PC_PORTAUDIO_FOUND)

mark_as_advanced(PC_PORTAUDIO_INCLUDE_DIRS PC_PORTAUDIO_LIBRARIES PC_PORTAUDIO_CFLAGS_OTHER)
if(PC_PORTAUDIO_FOUND)
    set(PORTAUDIO_FOUND ${PC_PORTAUDIO_FOUND})
    set(PORTAUDIO_INCLUDES ${PC_PORTAUDIO_INCLUDE_DIRS})
    set(PORTAUDIO_LIBRARIES ${PC_PORTAUDIO_LIBRARIES})

    if(NOT TARGET Portaudio::Portaudio)
        add_library(Portaudio::Portaudio SHARED IMPORTED)
        set_target_properties(Portaudio::Portaudio
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_PORTAUDIO_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_PORTAUDIO_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_PORTAUDIO_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_PORTAUDIO_LIBRARIES}"
        )
    endif()
endif()