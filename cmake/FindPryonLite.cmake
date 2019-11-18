# - Try to find  PryonLite
# Once done this will define
#  PRYON_LITE_FOUND - System has PryonLite
#  PRYON_LITE_INCLUDES - The PryonLite include directories
#  PRYON_LITE_LIBRARIES - The libraries needed to use PryonLite

find_path(PRYON_LITE_INCLUDES pryon_lite.h PATH_SUFFIXES pryon_lite)
find_library(PRYON_LITE_LIBRARIES libpryon_lite.so)

find_package_handle_standard_args(PRYON_LITE DEFAULT_MSG
        PRYON_LITE_INCLUDES
        PRYON_LITE_LIBRARIES)
mark_as_advanced(PRYON_LITE_FOUND YOGA_INCLUDES YOGA_LIBRARIES)

if(PRYON_LITE_FOUND)
    if(NOT TARGET PryonLite::PryonLite)
        add_library(PryonLite::PryonLite SHARED IMPORTED)
        set_target_properties(PryonLite::PryonLite
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PRYON_LITE_INCLUDES}"
                IMPORTED_LINK_INTERFACE_LIBRARIES "${PRYON_LITE_LIBRARIES}"
        )
    endif()
endif()
