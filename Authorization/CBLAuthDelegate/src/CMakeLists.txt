find_package(AlexaClientSDK REQUIRED)
find_package(LibCURL REQUIRED)
find_package(Threads REQUIRED)

set(MODULE_NAME CBLAuthDelegate)

add_definitions(-DACSDK_LOG_MODULE=cblAuthDelegate)
add_definitions(-DRAPIDJSON_HAS_STDSTRING)

add_library(${MODULE_NAME} SHARED
    CBLAuthDelegate.cpp
    CBLAuthDelegateConfiguration.cpp
    SQLiteCBLAuthDelegateStorage.cpp)

target_include_directories(${MODULE_NAME} PUBLIC ../include)

target_include_directories(${MODULE_NAME} PUBLIC AlexaClientSDK::AlexaClientSDK)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        AVSCommon
        RegistrationManager
        SQLiteStorage
        ${CURL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})

# install target
install(TARGETS ${MODULE_NAME} DESTINATION lib/)