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