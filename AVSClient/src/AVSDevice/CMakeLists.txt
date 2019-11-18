find_package(AlexaClientSDK REQUIRED)
find_package(GStreamer REQUIRED)
find_package(Portaudio)
find_package(PryonLite)
find_package(Threads REQUIRED)
find_package(WPEFramework REQUIRED)

set(MODULE_NAME AVSDevice)

set(WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES)
list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES
    ../Module.cpp
    ../ThunderLogger.cpp
    AVSDevice.cpp
    ConnectionObserver.cpp
    ConsolePrinter.cpp
    ConsoleReader.cpp
    GuiRenderer.cpp
    InteractionManager.cpp
    KeywordObserver.cpp
    LocaleAssetsManager.cpp
    SampleEqualizerModeController.cpp
    ThunderInputManager.cpp
    UIManager.cpp
    UserInputManager.cpp
)

if(PLUGIN_AVS_ENABLE_A4B)
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES RevokeAuthorizationObserver.cpp)
    add_definitions(-DENABLE_REVOKE_AUTH)
endif()

if(PORTAUDIO_FOUND)
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

if(PLUGIN_AVS_ENABLE_KWD)
    if(PRYON_LITE_FOUND)
        list(APPEND WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES ../PryonKeywordDetector.cpp)
    endif()
endif()

add_library(${MODULE_NAME} ${WPEFRAMEWORK_PLUGIN_AVS_AVSDEVICE_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ../../include/
        ../../include/AVSDevice
        AlexaClientSDK::AlexaClientSDK)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        ${ALEXA_CLIENT_SDK_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        rt
        m
        asound
        atomic)

if(PLUGIN_AVS_ENABLE_KWD)
    if(PRYON_LITE_FOUND)
        target_include_directories(${MODULE_NAME} PUBLIC ${PRYON_LITE_INCLUDES})
        target_link_libraries(${MODULE_NAME} PRIVATE ${PRYON_LITE_LIBRARIES})
    else()
        message(FATAL_ERROR "Missing pryon_lite library!")
    endif()
endif()

if(GSTREAMER_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC ${GSTREAMER_INCLUDES})
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${GSTREAMER_LIBRARIES}
            MediaPlayer)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

if(PORTAUDIO_FOUND)
    target_include_directories(${MODULE_NAME} PUBLIC Portaudio::Portaudio)
    target_link_libraries(${MODULE_NAME} PRIVATE ${PORTAUDIO_LIBRARIES})
    add_definitions(-DPORTAUDIO)
endif()

install(TARGETS ${MODULE_NAME} DESTINATION lib/)
