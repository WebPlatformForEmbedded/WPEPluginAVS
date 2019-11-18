/*
 * Copyright 2017-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include "SmartScreen.h"

#include <ContextManager/ContextManager.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSynchronizer.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>

#include "ConnectionObserver.h"
#include "KeywordObserver.h"

#ifdef ENABLE_REVOKE_AUTH
#include "RevokeAuthorizationObserver.h"
#endif

#ifdef ENABLE_PCC
#include "PhoneCaller.h"
#endif

#if defined(KWD) && !defined(KWD_PRYON)
#include <KWDProvider/KeywordDetectorProvider.h>
#elif defined(KWD) && defined(KWD_PRYON)
#include "PryonKeywordDetector.h"
#endif

#ifdef PORTAUDIO
#include <PortAudioMicrophoneWrapper.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#endif

#ifdef ANDROID
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
#include <AndroidUtilities/AndroidSLESEngine.h>
#endif

#ifdef ANDROID_MEDIA_PLAYER
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#include <AndroidSLESMediaPlayer/AndroidSLESSpeaker.h>
#endif

#ifdef ANDROID_MICROPHONE
#include <AndroidUtilities/AndroidSLESMicrophone.h>
#endif

#ifdef ANDROID_LOGGER
#include <AndroidUtilities/AndroidLogger.h>
#endif

#endif

#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <Bluetooth/SQLiteBluetoothStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <SampleEqualizerModeController.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <EqualizerImplementations/EqualizerController.h>
#include <EqualizerImplementations/InMemoryEqualizerConfiguration.h>
#include <EqualizerImplementations/MiscDBEqualizerStorage.h>
#include <EqualizerImplementations/SDKConfigEqualizerConfiguration.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <fstream>

#include <Communication/WebSocketServer.h>

#include "AplCoreEngineSDKLogBridge.h"
#include "AplCoreGuiRenderer.h"
#include "JsonUIManager.h"
#include "LocaleAssetsManager.h"

namespace alexaSmartScreenSDK {
namespace sampleApp {

SERVICE_REGISTRATION(SmartScreen, 1, 0);

/**
 * WebSocket interface to listen on.
 * WARNING: If this is changed to listen on a publicly accessible interface then additional
 * security precautions will need to be taken to secure client connections and authenticate
 * connecting clients.
 */
static const std::string DEFAULT_WEBSOCKET_INTERFACE = "127.0.0.1";

/// WebSocket port to listen on.
static const int DEFAULT_WEBSOCKET_PORT = 8933;

/// The sample rate of microphone audio data.
static const unsigned int SAMPLE_RATE_HZ = 16000;

/// The number of audio channels.
static const unsigned int NUM_CHANNELS = 1;

/// The size of each word within the stream.
static const size_t WORD_SIZE = 2;

/// The maximum number of readers of the stream.
static const size_t MAX_READERS = 10;

/// The amount of audio data to keep in the ring buffer.
static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);

/// The size of the ring buffer.
static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

/// Key for the root node value containing configuration values for SampleApp.
static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");

/// Key for the root node value containing configuration values for Equalizer.
static const std::string EQUALIZER_CONFIG_KEY("equalizer");

/// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");

/// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY("endpoint");

/// Key for setting the interface which websockets will bind to @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");

/// Key for setting the port number which websockets will listen on @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string WEBSOCKET_PORT_KEY("websocketPort");

/// Key for setting the certificate file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_CERTIFICATE("websocketCertificate");

/// Key for setting the private key file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG configuration
/// node.
static const std::string WEBSOCKET_PRIVATE_KEY("websocketPrivateKey");

/// Key for setting the certificate authority file for use by websockets when SSL is enabled, @c SAMPLE_APP_CONFIG
/// configuration node.
static const std::string WEBSOCKET_CERTIFICATE_AUTHORITY("websocketCertificateAuthority");

// String to indentify if input should be set to portaudio
static constexpr const char* PORTAUDIO_CALLSIGN("PORTAUDIO");

using namespace alexaClientSDK;
using namespace alexaClientSDK::capabilityAgents::externalMediaPlayer;

using namespace smartScreenSDKInterfaces;

/// The @c m_playerToMediaPlayerMap Map of the adapter to their speaker-type and MediaPlayer creation methods.
std::unordered_map<std::string, SmartScreen::SpeakerTypeAndCreateFunc>
    SmartScreen::m_playerToMediaPlayerMap;

/// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
std::unordered_map<std::string, ExternalMediaPlayer::AdapterCreateFunction> SmartScreen::m_adapterToCreateFuncMap;

/// String to identify log entries originating from this file.
static const std::string TAG("SmartScreen");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// A set of all log levels.
static const std::set<alexaClientSDK::avsCommon::utils::logger::Level> allLevels = {
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG9,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG8,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG7,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG6,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG5,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG4,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG3,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG2,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG1,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0,
    alexaClientSDK::avsCommon::utils::logger::Level::INFO,
    alexaClientSDK::avsCommon::utils::logger::Level::WARN,
    alexaClientSDK::avsCommon::utils::logger::Level::ERROR,
    alexaClientSDK::avsCommon::utils::logger::Level::CRITICAL,
    alexaClientSDK::avsCommon::utils::logger::Level::NONE};

/**
 * Gets a log level consumable by the SDK based on the user input string for log level.
 *
 * @param userInputLogLevel The string to be parsed into a log level.
 * @return The log level. This will default to NONE if the input string is not properly parsable.
 */
static alexaClientSDK::avsCommon::utils::logger::Level getLogLevelFromUserInput(std::string userInputLogLevel) {
    std::transform(userInputLogLevel.begin(), userInputLogLevel.end(), userInputLogLevel.begin(), ::toupper);
    return alexaClientSDK::avsCommon::utils::logger::convertNameToLevel(userInputLogLevel);
}

/**
 * Allows the process to ignore the SIGPIPE signal.
 * The SIGPIPE signal may be received when the application performs a write to a closed socket.
 * This is a case that arises in the use of certain networking libraries.
 *
 * @return true if the action for handling SIGPIPEs was correctly set to ignore, else false.
 */
static bool ignoreSigpipeSignals() {
#ifndef NO_SIGPIPE
    if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }
#endif
    return true;
}

bool SmartScreen::Initialize(
    WPEFramework::PluginHost::IShell *service,
    const std::string& alexaClientConfig,
    const std::string& smartScreenConfig,
    const std::string& pathToInputFolder,
    const std::string& audiosource,
    const bool enableKWD,
    const std::string& logLevel) {
    TRACE_L1("Initializing SmartScreen...");

    return initialize(service, alexaClientConfig, smartScreenConfig, pathToInputFolder, audiosource, enableKWD, logLevel);
}

bool SmartScreen::Deinitialize() {
    return true;
}

void SmartScreen::StateChange(WPEFramework::PluginHost::IShell* audiosource) {
    if(m_thunderVoiceHandler) {
        m_thunderVoiceHandler->stateChange(audiosource);
    }
}

WPEFramework::Exchange::IAVSController* SmartScreen::Controller() {
    return nullptr;
}

SmartScreen::AdapterRegistration::AdapterRegistration(
    const std::string& playerId,
    ExternalMediaPlayer::AdapterCreateFunction createFunction) {
    if (m_adapterToCreateFuncMap.find(playerId) != m_adapterToCreateFuncMap.end()) {
        ACSDK_WARN(LX("Adapter already exists").d("playerID", playerId));
    }

    m_adapterToCreateFuncMap[playerId] = createFunction;
}

SmartScreen::MediaPlayerRegistration::MediaPlayerRegistration(
    const std::string& playerId,
    avsCommon::sdkInterfaces::SpeakerInterface::Type speakerType,
    MediaPlayerCreateFunction createFunction) {
    if (m_playerToMediaPlayerMap.find(playerId) != m_playerToMediaPlayerMap.end()) {
        ACSDK_WARN(LX("MediaPlayer already exists").d("playerId", playerId));
    }

    m_playerToMediaPlayerMap[playerId] =
        std::pair<avsCommon::sdkInterfaces::SpeakerInterface::Type, MediaPlayerCreateFunction>(
            speakerType, createFunction);
}

alexaClientSDK::sampleApp::AVSClientRC SmartScreen::run() {
    return m_guiClient->run();
}

SmartScreen::~SmartScreen() {
    if (m_guiManager) {
        m_guiManager->shutdown();
    }

    if (m_guiClient) {
        m_guiClient->shutdown();
    }

    if (m_capabilitiesDelegate) {
        m_capabilitiesDelegate->shutdown();
    }

    // First clean up anything that depends on the the MediaPlayers.
    m_externalMusicProviderMediaPlayersMap.clear();

    // Now it's safe to shut down the MediaPlayers.
    for (auto& mediaPlayer : m_adapterMediaPlayers) {
        mediaPlayer->shutdown();
    }
    if (m_speakMediaPlayer) {
        m_speakMediaPlayer->shutdown();
    }
    if (m_audioMediaPlayer) {
        m_audioMediaPlayer->shutdown();
    }
    if (m_alertsMediaPlayer) {
        m_alertsMediaPlayer->shutdown();
    }
    if (m_notificationsMediaPlayer) {
        m_notificationsMediaPlayer->shutdown();
    }
    if (m_bluetoothMediaPlayer) {
        m_bluetoothMediaPlayer->shutdown();
    }
    if (m_systemSoundMediaPlayer) {
        m_systemSoundMediaPlayer->shutdown();
    }
    if (m_ringtoneMediaPlayer) {
        m_ringtoneMediaPlayer->shutdown();
    }

    avsCommon::avs::initialization::AlexaClientSDKInit::uninitialize();
}

bool SmartScreen::createMediaPlayersForAdapters(
    std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
    std::shared_ptr<smartScreenClient::EqualizerRuntimeSetup> equalizerRuntimeSetup,
    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers) {
#ifdef GSTREAMER_MEDIA_PLAYER
    bool equalizerEnabled = nullptr != equalizerRuntimeSetup;
    for (auto& entry : m_playerToMediaPlayerMap) {
        auto mediaPlayer = entry.second.second(
            httpContentFetcherFactory, equalizerEnabled, entry.second.first, entry.first + "MediaPlayer");
        if (mediaPlayer) {
            m_externalMusicProviderMediaPlayersMap[entry.first] = mediaPlayer;
            m_externalMusicProviderSpeakersMap[entry.first] = mediaPlayer;
            additionalSpeakers.push_back(
                std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer));
            m_adapterMediaPlayers.push_back(mediaPlayer);
            if (equalizerEnabled) {
                equalizerRuntimeSetup->addEqualizer(mediaPlayer);
            }
        } else {
            ACSDK_CRITICAL(LX("Failed to create mediaPlayer").d("playerId", entry.first));
            return false;
        }
    }

    return true;
#else
    if (!m_playerToMediaPlayerMap.empty()) {
        // TODO(ACSDK-1622) Add support to external media players on android.
        ACSDK_CRITICAL(LX("Failed to create media players").d("reason", "unsupportedOperation"));
        return false;
    }
    return true;
#endif
}

bool SmartScreen::initialize(
    WPEFramework::PluginHost::IShell *service,
    const std::string& alexaClientConfig,
    const std::string& smartScreenConfig,
    const std::string& pathToInputFolder,
    const std::string& audiosource,
    const bool enableKWD,
    const std::string& logLevel) {
    /*
     * Set up the SDK logging system to write to the SampleApp's ThunderLogger.  Also adjust the logging level
     * if requested.
     */
    std::shared_ptr<alexaClientSDK::avsCommon::utils::logger::Logger> thunderLogger = alexaClientSDK::avsCommon::utils::logger::getThunderLogger();

    avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;
    if (!logLevel.empty()) {
        logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaClientSDK::sampleApp::ThunderLogger::trace("Unknown log level input!");
            alexaClientSDK::sampleApp::ThunderLogger::trace("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                alexaClientSDK::sampleApp::ThunderLogger::trace(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        alexaClientSDK::sampleApp::ThunderLogger::trace(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
        thunderLogger->setLevel(logLevelValue);
    }

#ifdef ANDROID_LOGGER
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().initialize(
        std::make_shared<applicationUtilities::androidUtilities::AndroidLogger>(logLevelValue));
#else
    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().initialize(thunderLogger);
#endif

    std::vector<std::shared_ptr<std::istream>> configJsonStreams;
    if(!jsonConfigToStream(configJsonStreams, alexaClientConfig)){
        return false;
    }

#if defined(KWD_PRYON)
    if(!jsonConfigToStream(configJsonStreams, pathToInputFolder + "/localeToModels.json")){
        ACSDK_CRITICAL(LX("Failed to load localeToModels.json!"));
        return false;
    }
#endif

    jsonConfigToStream(configJsonStreams, smartScreenConfig);

    if (!avsCommon::avs::initialization::AlexaClientSDKInit::initialize(configJsonStreams)) {
        ACSDK_CRITICAL(LX("Failed to initialize SDK!"));
        return false;
    }

    auto config = alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot();
    auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

    auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

    // Creating the misc DB object to be used by various components.
    std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
        alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);

    // Creating Equalizer specific implementations

    auto equalizerConfigBranch = config[EQUALIZER_CONFIG_KEY];
    auto equalizerConfiguration = equalizer::SDKConfigEqualizerConfiguration::create(equalizerConfigBranch);
    std::shared_ptr<smartScreenClient::EqualizerRuntimeSetup> equalizerRuntimeSetup = nullptr;

    bool equalizerEnabled = false;
    if (equalizerConfiguration && equalizerConfiguration->isEnabled()) {
        equalizerEnabled = true;
        equalizerRuntimeSetup = std::make_shared<smartScreenClient::EqualizerRuntimeSetup>();
        auto equalizerStorage = equalizer::MiscDBEqualizerStorage::create(miscStorage);
        auto equalizerModeController = sampleApp::SampleEqualizerModeController::create();

        equalizerRuntimeSetup->setStorage(equalizerStorage);
        equalizerRuntimeSetup->setConfiguration(equalizerConfiguration);
        equalizerRuntimeSetup->setModeController(equalizerModeController);
    }

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    m_openSlEngine = applicationUtilities::androidUtilities::AndroidSLESEngine::create();
    if (!m_openSlEngine) {
        ACSDK_ERROR(LX("createAndroidMicFailed").d("reason", "failed to create engine"));
        return false;
    }
#endif

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker;
    std::tie(m_speakMediaPlayer, speakSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "SpeakMediaPlayer");
    if (!m_speakMediaPlayer || !speakSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for speech!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker;
    std::tie(m_audioMediaPlayer, audioSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        equalizerEnabled,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "AudioMediaPlayer");
    if (!m_audioMediaPlayer || !audioSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for content!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker;
    std::tie(m_notificationsMediaPlayer, notificationsSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "NotificationsMediaPlayer");
    if (!m_notificationsMediaPlayer || !notificationsSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for notifications!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker;
    std::tie(m_bluetoothMediaPlayer, bluetoothSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "BluetoothMediaPlayer");

    if (!m_bluetoothMediaPlayer || !bluetoothSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for bluetooth!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker;
    std::tie(m_ringtoneMediaPlayer, ringtoneSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "RingtoneMediaPlayer");
    if (!m_ringtoneMediaPlayer || !ringtoneSpeaker) {
        alexaClientSDK::sampleApp::ThunderLogger::log("Failed to create media player for ringtones!");
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker;
    std::tie(m_alertsMediaPlayer, alertsSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
        "AlertsMediaPlayer");
    if (!m_alertsMediaPlayer || !alertsSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for alerts!"));
        return false;
    }

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> systemSoundSpeaker;
    std::tie(m_systemSoundMediaPlayer, systemSoundSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "SystemSoundMediaPlayer");
    if (!m_systemSoundMediaPlayer || !systemSoundSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for system sound player!"));
        return false;
    }

#ifdef ENABLE_PCC
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> phoneSpeaker;
    std::shared_ptr<ApplicationMediaPlayer> phoneMediaPlayer;
    std::tie(phoneMediaPlayer, phoneSpeaker) = createApplicationMediaPlayer(
        httpContentFetcherFactory,
        false,
        avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
        "PhoneMediaPlayer");

    if (!phoneMediaPlayer || !phoneSpeaker) {
        ACSDK_CRITICAL(LX("Failed to create media player for phone!"));
        return false;
    }
#endif

    std::vector<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>> additionalSpeakers;

    if (!createMediaPlayersForAdapters(httpContentFetcherFactory, equalizerRuntimeSetup, additionalSpeakers)) {
        ACSDK_CRITICAL(LX("Could not create mediaPlayers for adapters"));
        return false;
    }

    auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();

    // Creating equalizers
    if (nullptr != equalizerRuntimeSetup) {
        equalizerRuntimeSetup->addEqualizer(m_audioMediaPlayer);
    }

    // Creating the alert storage object to be used for rendering and storing alerts.
    auto alertStorage =
        alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts());

    // Creating the message storage object to be used for storing message to be sent later.
    auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);

    // Creating notifications storage object to be used for storing notification indicators.

    auto notificationsStorage =
        alexaClientSDK::capabilityAgents::notifications::SQLiteNotificationsStorage::create(config);

    // Creating new device settings storage object to be used for storing AVS Settings.

    auto deviceSettingsStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);

    // Create HTTP Put handler
    std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPut> httpPut =
        avsCommon::utils::libcurlUtils::HttpPut::create();

    // Creating bluetooth storage object to be used for storing uuid to mac mappings for devices.

    auto bluetoothStorage = alexaClientSDK::capabilityAgents::bluetooth::SQLiteBluetoothStorage::create(config);

#ifdef KWD
    bool wakeWordEnabled = enableKWD;
#else
    bool wakeWordEnabled = false;
#endif

    /*
     * Create sample locale asset manager.
     */
    auto localeAssetsManager = LocaleAssetsManager::create(wakeWordEnabled);
    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Failed to create Locale Assets Manager!"));
        return false;
    }

    std::string APLVersion;
    std::string websocketInterface;
    sampleAppConfig.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);

    int websocketPortNumber;
    sampleAppConfig.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);

    // Create the websocket server that handles communications with websocket clients

    auto webSocketServer = std::make_shared<communication::WebSocketServer>(websocketInterface, websocketPortNumber);

#ifdef ENABLE_WEBSOCKET_SSL
    std::string sslCaFile;
    sampleAppConfig.getString(WEBSOCKET_CERTIFICATE_AUTHORITY, &sslCaFile);

    std::string sslCertificateFile;
    sampleAppConfig.getString(WEBSOCKET_CERTIFICATE, &sslCertificateFile);

    std::string sslPrivateKeyFile;
    sampleAppConfig.getString(WEBSOCKET_PRIVATE_KEY, &sslPrivateKeyFile);

    webSocketServer->setCertificateFile(sslCaFile, sslCertificateFile, sslPrivateKeyFile);
#endif

    m_guiClient = gui::GUIClient::create(webSocketServer, miscStorage);

    if (!m_guiClient) {
        ACSDK_CRITICAL(LX("Creation of GUIClient failed!"));
        return false;
    }

    auto aplCoreConnectionManager = std::make_shared<AplCoreConnectionManager>(m_guiClient);
    auto aplCoreGuiRenderer =
        std::make_shared<AplCoreGuiRenderer>(aplCoreConnectionManager, httpContentFetcherFactory);

    m_guiClient->setAplCoreConnectionManager(aplCoreConnectionManager);
    m_guiClient->setAplCoreGuiRenderer(aplCoreGuiRenderer);

    if (!m_guiClient->start()) {
        return false;
    }

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();

#ifdef ENABLE_PCC
    auto phoneCaller = std::make_shared<alexaSmartScreenSDK::sampleApp::PhoneCaller>();
#endif

    // Creating the deviceInfo object
    std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create(config);
    if (!deviceInfo) {
        ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
        return false;
    }

    // Creating the UI component that observes various components and prints to the console accordingly.

    auto userInterfaceManager = std::make_shared<alexaSmartScreenSDK::sampleApp::JsonUIManager>(
        std::static_pointer_cast<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface>(m_guiClient), deviceInfo);
    m_guiClient->setObserver(userInterfaceManager);

    APLVersion = m_guiClient->getMaxAPLVersion();

    // Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
    auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(config);
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate =
        authorization::cblAuthDelegate::CBLAuthDelegate::create(
            config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);

    if (!authDelegate) {
        ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
        return false;
    }

    /*
     * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
     * Capabilities API.
     */
    m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
        authDelegate, miscStorage, httpPut, customerDataManager, config, deviceInfo);

    if (!m_capabilitiesDelegate) {
        alexaClientSDK::sampleApp::ThunderLogger::log("Creation of CapabilitiesDelegate failed!");
        return false;
    }

    authDelegate->addAuthObserver(userInterfaceManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

    // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

    // Creating the InternetConnectionMonitor that will notify observers of internet connection status changes.

    auto internetConnectionMonitor =
        avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
    if (!internetConnectionMonitor) {
        ACSDK_CRITICAL(LX("Failed to create InternetConnectionMonitor"));
        return false;
    }

    /*
     * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
     * It is required for each of the capability agents so that they may provide their state just before any event is
     * fired off.
     */
    auto contextManager = contextManager::ContextManager::create();
    if (!contextManager) {
        ACSDK_CRITICAL(LX("Creation of ContextManager failed."));
        return false;
    }
    apl::LoggerFactory::instance().initialize(std::make_shared<AplCoreEngineSDKLogBridge>(AplCoreEngineSDKLogBridge()));
    /*
     * Create a factory for creating objects that handle tasks that need to be performed right after establishing
     * a connection to AVS.
     */
    auto postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create(contextManager);

    // Create a factory to create objects that establish a connection with AVS.

    auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
        std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
        postConnectSynchronizerFactory);

    // Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.

    size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
        BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
    auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
        alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);

    if (!sharedDataStream) {
        ACSDK_CRITICAL(LX("Failed to create shared data stream!"));
        return false;
    }

    /*
     * Create the BluetoothDeviceManager to communicate with the Bluetooth stack.
     */
    std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager;

    alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
    compatibleAudioFormat.sampleRateHz = SAMPLE_RATE_HZ;
    compatibleAudioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
    compatibleAudioFormat.numChannels = NUM_CHANNELS;
    compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
    compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

    /*
     * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
     * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
     * stream is used since this sample application will only have one microphone.
     */

    // Creating tap to talk audio provider
    bool tapAlwaysReadable = true;
    bool tapCanOverride = true;
    bool tapCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider tapToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        tapAlwaysReadable,
        tapCanOverride,
        tapCanBeOverridden);

    // Creating hold to talk audio provider
    bool holdAlwaysReadable = false;
    bool holdCanOverride = true;
    bool holdCanBeOverridden = false;

    alexaClientSDK::capabilityAgents::aip::AudioProvider holdToTalkAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
        holdAlwaysReadable,
        holdCanOverride,
        holdCanBeOverridden);

#ifdef KWD
    bool wakeAlwaysReadable = true;
    bool wakeCanOverride = false;
    bool wakeCanBeOverridden = true;

    alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider(
        sharedDataStream,
        compatibleAudioFormat,
        alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
        wakeAlwaysReadable,
        wakeCanOverride,
        wakeCanBeOverridden);
#endif

    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> aspInput = nullptr;
    std::shared_ptr<alexaClient::InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>> aspInputInteractionHandler = nullptr;

    if(audiosource == PORTAUDIO_CALLSIGN) {
#if defined(PORTAUDIO)
        aspInput = PortAudioMicrophoneWrapper::create(sharedDataStream);
#elif defined(ANDROID_MICROPHONE)
        aspInput = m_openSlEngine->createAndroidMicrophone(sharedDataStream);
#else
#error "No audio input provided"
#endif
    } else {
        aspInputInteractionHandler = alexaClient::InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::Create();
        if(!aspInputInteractionHandler) {
            ACSDK_CRITICAL(LX("Failed to create Interaction Handler!"));
            return false;
        }
        m_thunderVoiceHandler = alexaClient::ThunderVoiceHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::create(sharedDataStream, service, audiosource, aspInputInteractionHandler, compatibleAudioFormat);
        aspInput = m_thunderVoiceHandler;
        aspInput->startStreamingMicrophoneData();
    }
    if (!aspInput) {
        ACSDK_CRITICAL(LX("Failed to create Audio Signal Processor input!"));
        return false;
    }

#ifdef KWD
    // If wake word is enabled, then creating the GUI manager with a wake word audio provider.
    m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        aspInput,
        wakeWordAudioProvider);
#else
    // If wake word is not enabled, then creating the gui manager without a wake word audio provider.
    m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
        m_guiClient,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        aspInput,
        alexaClientSDK::capabilityAgents::aip::AudioProvider::null());
#endif  // KWD


    if(audiosource != PORTAUDIO_CALLSIGN) {
        if(aspInputInteractionHandler) {
            // register interactions that ThunderVoiceHandler may initiate
            if(!aspInputInteractionHandler->Initialize(m_guiManager)) {
                ACSDK_CRITICAL(LX("Failed to initialize interaction handler!"));
                return false;
            }
        }
    }
    /*
     * Creating the SmartScreenClient - this component serves as an out-of-box default object that instantiates and
     * "glues" together all the modules.
     */
    std::shared_ptr<smartScreenClient::SmartScreenClient> smartScreenClient =
        smartScreenClient::SmartScreenClient::create(
            deviceInfo,
            customerDataManager,
            m_externalMusicProviderMediaPlayersMap,
            m_externalMusicProviderSpeakersMap,
            m_adapterToCreateFuncMap,
            m_speakMediaPlayer,
            m_audioMediaPlayer,
            m_alertsMediaPlayer,
            m_notificationsMediaPlayer,
            m_bluetoothMediaPlayer,
            m_ringtoneMediaPlayer,
            m_systemSoundMediaPlayer,
            speakSpeaker,
            audioSpeaker,
            alertsSpeaker,
            notificationsSpeaker,
            bluetoothSpeaker,
            ringtoneSpeaker,
            systemSoundSpeaker,
            additionalSpeakers,
#ifdef ENABLE_PCC
            phoneSpeaker,
            phoneCaller,
#endif
            equalizerRuntimeSetup,
            audioFactory,
            authDelegate,
            std::move(alertStorage),
            std::move(messageStorage),
            std::move(notificationsStorage),
            std::move(deviceSettingsStorage),
            std::move(bluetoothStorage),
            miscStorage,
            {userInterfaceManager},
            {userInterfaceManager},
            std::move(internetConnectionMonitor),
            m_capabilitiesDelegate,
            contextManager,
            transportFactory,
            localeAssetsManager,
            /* systemTimezone*/ nullptr,
            firmwareVersion,
            true,
            nullptr,
            std::move(bluetoothDeviceManager),
            m_guiManager,
            APLVersion);

    if (!smartScreenClient) {
        ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
        return false;
    }

    // Creating wake word audio provider, if necessary
#ifdef KWD
    // This observer is notified any time a keyword is detected and notifies the SmartScreenClient to start recognizing.
    auto keywordObserver =
        std::make_shared<alexaSmartScreenSDK::sampleApp::KeywordObserver>(smartScreenClient, wakeWordAudioProvider);

#if defined(KWD_PRYON)
    m_keywordDetector = alexaClient::kwd::PryonKeywordDetector::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
#else
    m_keywordDetector = alexaClientSDK::kwd::KeywordDetectorProvider::create(
        sharedDataStream,
        compatibleAudioFormat,
        {keywordObserver},
        std::unordered_set<
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
        pathToInputFolder);
#endif
    if (!m_keywordDetector) {
        ACSDK_CRITICAL(LX("Failed to create keyword detector!"));
        return false;
    }
#endif  // KWD

    smartScreenClient->addSpeakerManagerObserver(userInterfaceManager);
    smartScreenClient->addNotificationsObserver(userInterfaceManager);
    smartScreenClient->addTemplateRuntimeObserver(m_guiManager);
    smartScreenClient->addAlexaPresentationObserver(m_guiManager);
    smartScreenClient->addAlexaDialogStateObserver(m_guiManager);
    smartScreenClient->addAudioPlayerObserver(m_guiManager);
    m_guiManager->setClient(smartScreenClient);
    m_guiClient->setGUIManager(m_guiManager);

#ifdef ENABLE_REVOKE_AUTH
    // Creating the revoke authorization observer.
    auto revokeObserver =
        std::make_shared<alexaSmartScreenSDK::sampleApp::RevokeAuthorizationObserver>(client->getRegistrationManager());
    client->addRevokeAuthorizationObserver(revokeObserver);
#endif

    smartScreenClient->getRegistrationManager()->addObserver(m_guiClient);

    authDelegate->addAuthObserver(m_guiClient);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);
    m_capabilitiesDelegate->addCapabilitiesObserver(smartScreenClient);

    // Connect once configuration is all set.
    std::string endpoint;
    sampleAppConfig.getString(ENDPOINT_KEY, &endpoint);

    smartScreenClient->connect(m_capabilitiesDelegate, endpoint);

    return true;
}

std::pair<std::shared_ptr<ApplicationMediaPlayer>, std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface>>
SmartScreen::createApplicationMediaPlayer(
    std::shared_ptr<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory> httpContentFetcherFactory,
    bool enableEqualizer,
    avsCommon::sdkInterfaces::SpeakerInterface::Type type,
    const std::string& name) {
#ifdef GSTREAMER_MEDIA_PLAYER
    /*
     * For the SDK, the MediaPlayer happens to also provide volume control functionality.
     * Note the externalMusicProviderMediaPlayer is not added to the set of SpeakerInterfaces as there would be
     * more actions needed for these beyond setting the volume control on the MediaPlayer.
     */
    auto mediaPlayer =
        alexaClientSDK::mediaPlayer::MediaPlayer::create(httpContentFetcherFactory, enableEqualizer, type, name);
    return {mediaPlayer,
            std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer)};
#elif defined(ANDROID_MEDIA_PLAYER)
    auto mediaPlayer = mediaPlayer::android::AndroidSLESMediaPlayer::create(
        httpContentFetcherFactory,
        m_openSlEngine,
        type,
        enableEqualizer,
        mediaPlayer::android::PlaybackConfiguration(),
        name);
    if (!mediaPlayer) {
        return {nullptr, nullptr};
    }
    auto speaker = mediaPlayer->getSpeaker();
    return {std::move(mediaPlayer), speaker};
#endif
}

bool SmartScreen::jsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile)
{
    if (configFile.empty()) {
        alexaClientSDK::sampleApp::ThunderLogger::log("Config filename is empty!");
        return false;
    }

    auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
    if (!configInFile->good()) {
        ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
        alexaClientSDK::sampleApp::ThunderLogger::log("Failed to read config file " + configFile);
        return false;
    }

    streams.push_back(configInFile);
}

}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK
