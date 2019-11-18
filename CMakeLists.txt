cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(WPEFramework REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)

set(PLUGIN_AVS_NAME "AVS" CACHE STRING "The plugin name")
set(PLUGIN_AVS_AUTOSTART false CACHE STRING "Autostart plugin (true/false)")
set(PLUGIN_AVS_PLATFORM "rpi3" CACHE STRING "Platform name (currently only rpi3)")
set(PLUGIN_AVS_DATA_PATH "/usr/share/WPEFramework/" CACHE STRING "Path were AVS data will be stored")
set(PLUGIN_AVS_ALEXA_CLIENT_CONFIG "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/AlexaClientSDKConfig.json" CACHE STRING "Path to AlexaClientSDKConfig")
set(PLUGIN_AVS_SMART_SCREEN_CONFIG "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/SmartScreenSDKConfig.json" CACHE STRING "Path to SmartScreenSDKConfig")
set(PLUGIN_AVS_LOG_LEVEL "DEBUG0" CACHE STRING "Default log level for the SDK")
set(PLUGIN_AVS_ENABLE_THUNDER_INPUT_MANAGER ON CACHE BOOL "Enable Thunder input manager instead of default one")
set(PLUGIN_AVS_AUDIOSOURCE "PORTAUDIO" CACHE STRING "Callsign of plugin that provides voice input or PORTAUDIO")
set(PLUGIN_AVS_ENABLE_PORTAUDIO_SUPPORT ON CACHE BOOL "Enable audio input from PortAudio library")
set(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT ON CACHE BOOL "Compile in the Smart Screen support")
set(PLUGIN_AVS_ENABLE_SMART_SCREEN true CACHE STRING "Enable the Smart Screen support in the runtime (true/false)")
set(PLUGIN_AVS_ENABLE_KWD_SUPPORT ON CACHE BOOL "Compile in the Pryon Keyword Detection engine")
set(PLUGIN_AVS_ENABLE_KWD true CACHE STRING "Enable the Pryon Keyword Detection engine in the runtime (true/false)")
set(PLUGIN_AVS_KWD_MODELS_PATH "${PLUGIN_AVS_DATA_PATH}/${PLUGIN_AVS_NAME}/models" CACHE STRING "Path to KWD input directory")
set(PLUGIN_AVS_ENABLE_A4B OFF CACHE BOOL "Enable Alexa 4 Buisness (revoke authorisation)")

# TODO: remove me ;)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

set(MODULE_NAME ${NAMESPACE}${PLUGIN_AVS_NAME})
set(PLUGIN_NAME ${PLUGIN_AVS_NAME})
string(TOLOWER ${NAMESPACE} STORAGE_DIRECTORY)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)
add_definitions(-DGSTREAMER_MEDIA_PLAYER)

add_definitions(-DACSDK_LOG_SINK=Thunder)

if(PLUGIN_AVS_ENABLE_THUNDER_INPUT_MANAGER)
    add_definitions(-DENABLE_THUNDER_INPUT_MANAGER)
endif()

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    add_definitions(-DENABLE_SMART_SCREEN_SUPPORT)
endif()

if(PLUGIN_AVS_ENABLE_KWD)
    add_definitions(-DKWD)
    add_definitions(-DKWD_PRYON)
endif()

add_library(${MODULE_NAME}
    SHARED
        Module.cpp
        AVS.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES)

target_include_directories(${MODULE_NAME}
    PUBLIC
        AVSClient/include/)

add_subdirectory("Authorization")
add_subdirectory("AVSClient")
add_subdirectory("Integration")


target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        AVSDevice)

if(PLUGIN_AVS_ENABLE_SMART_SCREEN_SUPPORT)
    target_link_libraries(${MODULE_NAME} PRIVATE SmartScreen)
endif()

install(TARGETS ${MODULE_NAME}
    DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})