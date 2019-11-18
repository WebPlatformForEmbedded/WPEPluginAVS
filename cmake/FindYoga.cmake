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
