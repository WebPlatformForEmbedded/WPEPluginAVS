# - Try to find Asio
# Once done this will define
#  ASIO_FOUND - System has Asio
#  ASIO_INCLUDES - The Asio include directories

find_path(ASIO_INCLUDES asio.hpp)

find_package_handle_standard_args(ASIO DEFAULT_MSG
        ASIO_INCLUDES)
mark_as_advanced(ASIO_INCLUDES)

if(ASIO_FOUND)
    if(NOT TARGET Asio::Asio)
        add_library(Asio::Asio SHARED IMPORTED)
        set_target_properties(Asio::Asio
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ASIO_INCLUDES}"
        )
    endif()
endif()
