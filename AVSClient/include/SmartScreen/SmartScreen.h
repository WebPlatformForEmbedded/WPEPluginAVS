# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "ThunderLogger.h"
#include "SmartScreenClient/EqualizerRuntimeSetup.h"
#include "AVSClientRC.h"
#include "ThunderVoiceHandler.h"

#include "Module.h"
#include <WPEFramework/interfaces/IAVSClient.h>

#ifdef KWD
#include <KWD/AbstractKeywordDetector.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#elif defined(ANDROID_MEDIA_PLAYER)
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#endif

#include "FocusBridge.h"
#include "AplCoreGuiRenderer.h"
#include "GUI/GUIClient.h"
#include "GUI/GUIManager.h"

#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>

namespace alexaSmartScreenSDK {
namespace sampleApp {

#ifdef GSTREAMER_MEDIA_PLAYER
using ApplicationMediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer;
#elif defined(ANDROID_MEDIA_PLAYER)
using ApplicationMediaPlayer = mediaPlayer::android::AndroidSLESMediaPlayer;
#endif

/// Class to manage the top-level components of the AVS Client Application
class SmartScreen : public WPEFramework::Exchange::IAVSClient {
public:
    /**
     * Default constructor
     */
    SmartScreen() = default;

    /**
     * Initialize the AVS Device application
     *
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param alexaClientConfig The configuration file path for the Alexa Client.
     * @param smartScreenConfig The configuration file path for Smart Screen Client.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param audiosource The callsign of the nano service proividing audio input or PORTAUDIO for USB microphone.
     * @param enableKWD Enable Keyword Detection in the pipeline.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, otherwise @c false.
     */
    bool Initialize(WPEFramework::PluginHost::IShell *service, const std::string& alexaClientConfig, const std::string& smartScreenConfig, const std::string& pathToInputFolder, const std::string& audiosource, const bool enableKWD, const std::string& logLevel) override;

    /**
     * Deinitialize the AVS Device application
     *
     * @return @c true if deinitialization succedded, otherwise @c false.
     */
    bool Deinitialize() override;

    /**
     * Received on state change of audiosource callsign
     *
     * @param audiosource A IShell information about audiosource callsign
     */
    void StateChange(WPEFramework::PluginHost::IShell* audiosource) override;

    WPEFramework::Exchange::IAVSController* Controller() override;

    BEGIN_INTERFACE_MAP(SmartScreen)
        INTERFACE_ENTRY(WPEFramework::Exchange::IAVSClient)
    END_INTERFACE_MAP

    /**
     * Runs the application, blocking until the user asks the application to quit or a device reset is triggered.
     *
     * @return Returns a @c AVSClientRC.
     */
    alexaClientSDK::sampleApp::AVSClientRC run();

    /// Destructor which manages the @c SmartScreen shutdown sequence.
    ~SmartScreen();

    /**
     * Method to create mediaPlayers for the optional music provider adapters plugged into the SDK.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param equalizerRuntimeSetup Equalizer runtime setup to register equalizers
     * @param additionalSpeakers The speakerInterface to add the created mediaPlayer.
     * @return @c true if the mediaPlayer of all the registered adapters could be created @c false otherwise.
     */
    bool createMediaPlayersForAdapters(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>
            httpContentFetcherFactory,
        std::shared_ptr<smartScreenClient::EqualizerRuntimeSetup> equalizerRuntimeSetup,
        std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>& additionalSpeakers);

    /**
     * Instances of this class register ExternalMediaAdapters. Each adapter registers itself by instantiating
     * a static instance of the below class supplying their business name and creator method.
     */
    class AdapterRegistration {
    public:
        /**
         * Register an @c ExternalMediaAdapter for use by @c ExternalMediaPlayer.
         * @param playerId The @c playerId identifying the @c ExtnalMediaAdapter to register.
         * @param createFunction The function to use to create instances of the specified @c ExternalMediaAdapter.
         */
        AdapterRegistration(
            const std::string& playerId,
            alexaClientSDK::capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreateFunction
                createFunction);
    };

    /**
     * Signature of functions to create a MediaPlayer.
     *
     * @param httpContentFetcherFactory The HTTPContentFetcherFactory to be used while creating the mediaPlayers.
     * @param type The type of the SpeakerInterface.
     * @param name The name of the MediaPlayer instance.
     * @return Return shared pointer to the created MediaPlayer instance.
     */
    using MediaPlayerCreateFunction = std::shared_ptr<ApplicationMediaPlayer> (*)(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>
            contentFetcherFactory,
        bool enableEqualizer,
        alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        std::string name);

    /**
     * Instances of this class register MediaPlayers to be created. Each third-party adapter registers a mediaPlayer
     * for itself by instantiating a static instance of the below class supplying their business name, speaker interface
     * type and creator method.
     */
    class MediaPlayerRegistration {
    public:
        /**
         * Register a @c MediaPlayer for use by a music provider adapter.
         *
         * @param playerId The @c playerId identifying the @c ExternalMediaAdapter to register.
         * @speakerType The SpeakerType of the mediaPlayer to be created.
         * @param createFunction The function to use to create instances of the mediaPlayer to use for the player.
         */
        MediaPlayerRegistration(
            const std::string& playerId,
            alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type speakerType,
            MediaPlayerCreateFunction createFunction);
    };

private:
    /**
     * Initialize a SmartScreen.
     *
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param alexaClientConfig The configuration file path for the Alexa Client.
     * @param smartScreenConfig The configuration file path for Smart Screen.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param audiosource The callsign of the nano service proividing audio input or PORTAUDIO for USB microphone.
     * @param enableKWD Enable Keyword Detection in the pipeline.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(
        WPEFramework::PluginHost::IShell *service,
        const std::string& alexaClientConfig,
        const std::string& smartScreenConfig,
        const std::string& pathToInputFolder,
        const std::string& audiosource,
        const bool enableKWD,
        const std::string& logLevel = "");

    /**
     * Create an application media player.
     *
     * @param contentFetcherFactory Used to create objects that can fetch remote HTTP content.
     * @param enableEqualizer Flag indicating if equalizer should be enabled for this media player.
     * @param type The type used to categorize the speaker for volume control.
     * @param name The media player instance name used for logging purpose.
     * @return A pointer to the @c ApplicationMediaPlayer and to its speaker if it succeeds; otherwise, return @c
     * nullptr.
     */
    std::pair<
        std::shared_ptr<ApplicationMediaPlayer>,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>
    createApplicationMediaPlayer(
        std::shared_ptr<alexaClientSDK::avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>
            httpContentFetcherFactory,
        bool enableEqualizer,
        alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type type,
        const std::string& name);

    /**
     * Creates a istream from config file path and adds it to the vector.
     *
     * @param streams Vector of streams.
     * @param configFile config file path.
     * @return true wheter creation was sucessfull.
     */
    bool jsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile);

    /// The @c GUIClient
    std::shared_ptr<gui::GUIClient> m_guiClient;

    std::shared_ptr<gui::GUIManager> m_guiManager;

    /// The map of the adapters and their mediaPlayers.
    std::
        unordered_map<std::string, std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface>>
            m_externalMusicProviderMediaPlayersMap;

    /// The map of the adapters and their mediaPlayers.
    std::unordered_map<std::string, std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>
        m_externalMusicProviderSpeakersMap;

    /// The vector of mediaPlayers for the adapters.
    std::vector<std::shared_ptr<ApplicationMediaPlayer>> m_adapterMediaPlayers;

    /// The @c MediaPlayer used by @c SpeechSynthesizer.
    std::shared_ptr<ApplicationMediaPlayer> m_speakMediaPlayer;

    /// The @c MediaPlayer used by @c AudioPlayer.
    std::shared_ptr<ApplicationMediaPlayer> m_audioMediaPlayer;

    /// The @c MediaPlayer used by @c Alerts.
    std::shared_ptr<ApplicationMediaPlayer> m_alertsMediaPlayer;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<ApplicationMediaPlayer> m_notificationsMediaPlayer;

    /// The @c MediaPlayer used by @c Bluetooth.
    std::shared_ptr<ApplicationMediaPlayer> m_bluetoothMediaPlayer;

    /// The @c MediaPlayer used by @c SystemSoundPlayer.
    std::shared_ptr<ApplicationMediaPlayer> m_systemSoundMediaPlayer;

    /// The @c CapabilitiesDelegate used by the client.
    std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;

    /// The @c MediaPlayer used by @c NotificationsCapabilityAgent.
    std::shared_ptr<ApplicationMediaPlayer> m_ringtoneMediaPlayer;

    using SpeakerTypeAndCreateFunc =
        std::pair<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface::Type, MediaPlayerCreateFunction>;

    /// The singleton map from @c playerId to @c MediaPlayerCreateFunction.
    static std::unordered_map<std::string, SpeakerTypeAndCreateFunc> m_playerToMediaPlayerMap;

    /// The singleton map from @c playerId to @c ExternalMediaAdapter creation functions.
    static alexaClientSDK::capabilityAgents::externalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap
        m_adapterToCreateFuncMap;

#ifdef KWD
    /// The Wakeword Detector which can wake up the client using audio input.
    std::unique_ptr<alexaClientSDK::kwd::AbstractKeywordDetector> m_keywordDetector;
#endif

#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    /// The android OpenSL ES engine used to create media players and microphone.
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESEngine> m_openSlEngine;
#endif

    std::shared_ptr<alexaClient::ThunderVoiceHandler<gui::GUIManager>> m_thunderVoiceHandler;
};

}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK
