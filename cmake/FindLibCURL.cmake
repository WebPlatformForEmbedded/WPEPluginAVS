# - Try to find  LibCURL
# Once done this will define
#  LIBCURL_FOUND - System has LibCURL
#  LIBCURL_INCLUDES - The LibCURL include directories
#  LIBCURL_LIBRARIES - The libraries needed to use LibCURL

find_package(PkgConfig)
pkg_check_modules(PC_LIBCURL libcurl)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PC_LIBCURL DEFAULT_MSG PC_LIBCURL_FOUND)

mark_as_advanced(PC_ALEXA_CLIENT_SDK_INCLUDE_DIRS PC_LIBCURL_LIBRARIES PC_LIBCURL_CFLAGS_OTHER)

if(PC_LIBCURL_FOUND)
    set(LIBCURL_FOUND ${PC_LIBCURL_FOUND})
    set(LIBCURL_INCLUDES ${PC_LIBCURL_INCLUDE_DIRS})
    set(LIBCURL_LIBRARIES ${PC_LIBCURL_LIBRARIES})

    if(NOT TARGET LibCURL::LibCURL)
        add_library(LibCURL::LibCURL SHARED IMPORTED)
        set_target_properties(LibCURL::LibCURL
            PROPERTIES
                INTERFACE_COMPILE_DEFINITIONS "${PC_LIBCURL_DEFINITIONS}"
                INTERFACE_COMPILE_OPTIONS "${PC_LIBCURL_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_LIBCURL_INCLUDE_DIR}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PC_LIBCURL_LIBRARIES}")
    endif()
endif()