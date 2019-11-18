find_package(APLCore REQUIRED)
find_package(AlexaClientSDK REQUIRED)
find_package(AlexaSmartScreenSDK REQUIRED)
find_package(Asio REQUIRED)
find_package(GStreamer REQUIRED)
find_package(Portaudio)
find_package(PryonLite)
find_package(Threads REQUIRED)
find_package(WPEFramework REQUIRED)
find_package(Websocketpp REQUIRED)
find_package(Yoga REQUIRED)

set(MODULE_NAME SmartScreen)

set(WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES)
list(APPEND WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES
    ../Module.cpp
    ../ThunderLogger.cpp
    AplCoreConnectionManager.cpp
    AplCoreEngineSDKLogBridge.cpp
    AplCoreGuiRenderer.cpp
    AplCoreMetrics.cpp
    AplCoreTextMeasurement.cpp
    ConnectionObserver.cpp
    ConsolePrinter.cpp
    FocusBridge.cpp
    GUI/GUIClient.cpp
    GUI/GUIManager.cpp
    GUILogBridge.cpp
    JsonUIManager.cpp
    KeywordObserver.cpp
    LocaleAssetsManager.cpp
    SampleEqualizerModeController.cpp
    SmartScreen.cpp
)

if(PORTAUDIO_FOUND)
    list(APPEND WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES PortAudioMicrophoneWrapper.cpp)
endif()

if(PLUGIN_AVS_ENABLE_KWD)
    if(PRYON_LITE_FOUND)
        list(APPEND WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES ../PryonKeywordDetector.cpp)
    endif()
endif()

add_library(${MODULE_NAME} ${WPEFRAMEWORK_PLUGIN_AVS_SMARTSCREEN_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON)

target_include_directories(${MODULE_NAME}
    PUBLIC
        ../../include/
        ../../include/SmartScreen
        AlexaClientSDK::AlexaClientSDK
        AlexaSmartScreenSDK::AlexaSmartScreenSDK)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        AlexaPresentation
        SmartScreenTemplateRunTime
        VisualCharacteristics
        Communication
        SmartScreenClient
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

if(APL_CORE_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE APLCore::APLCore)
    add_definitions(-DAPL_CORE)
    target_link_libraries(${MODULE_NAME} PRIVATE ${APL_CORE_LIBRARIES})
else()
    message(FATAL_ERROR "MISSING APL Core Library")
endif()

if(YOGA_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Yoga::Yoga)
    target_link_libraries(${MODULE_NAME} PRIVATE ${YOGA_LIBRARIES})
else()
    message(FATAL_ERROR "MISSING Yoga Library")
endif()

if(WEBSOCKETPP_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Websocketpp::Websocketpp)
else()
    message(FATAL_ERROR "MISSING Websocketpp Library")
endif()

if(ASIO_FOUND)
    target_include_directories(${MODULE_NAME} PRIVATE Asio::Asio)
    add_definitions(-DASIO_STANDALONE)
else()
    message(FATAL_ERROR "MISSING Asio (Standalone) Library")
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
