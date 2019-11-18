add_subdirectory(AVSDevice)

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    add_subdirectory(SmartScreen)
endif()