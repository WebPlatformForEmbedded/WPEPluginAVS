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
