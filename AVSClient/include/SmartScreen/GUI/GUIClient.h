/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_SMART_SCREEN_SDK_SAMPLEAPP_GUI_INCLUDE_SAMPLEAPP_GUI_GUICLIENT_H
#define ALEXA_SMART_SCREEN_SDK_SAMPLEAPP_GUI_INCLUDE_SAMPLEAPP_GUI_GUICLIENT_H

// TODO: Tidy up core to prevent this (ARC-917)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma push_macro("DEBUG")
#pragma push_macro("TRUE")
#pragma push_macro("FALSE")
#undef DEBUG
#undef TRUE
#undef FALSE
#include <apl/content/content.h>
#pragma pop_macro("DEBUG")
#pragma pop_macro("TRUE")
#pragma pop_macro("FALSE")
#pragma GCC diagnostic pop
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <string>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/FocusState.h>
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Timing/Timer.h>
#include <RegistrationManager/RegistrationObserverInterface.h>

#include <apl/action/action.h>
#include <apl/engine/rootcontext.h>

#include <SmartScreenSDKInterfaces/ActivityEvent.h>
#include <SmartScreenSDKInterfaces/AudioPlayerInfo.h>
#include <SmartScreenSDKInterfaces/GUIClientInterface.h>
#include <SmartScreenSDKInterfaces/GUIServerInterface.h>
#include <SmartScreenSDKInterfaces/MessageInterface.h>
#include <SmartScreenSDKInterfaces/MessagingServerInterface.h>
#include <SmartScreenSDKInterfaces/NavigationEvent.h>

#include "../GUILogBridge.h"
#include "../AplCoreConnectionManager.h"
#include "../AplCoreGuiRenderer.h"
#include "AVSClientRC.h"
#include "../FocusBridge.h"

namespace alexaSmartScreenSDK {
namespace sampleApp {
namespace gui {
/**
 * Manages all GUI related operations to be called from the GUI and the SDK
 * Encapsulates APL core Client implementation and APL Core integration point.
 */
class GUIClient
        : public smartScreenSDKInterfaces::MessagingServerInterface
        , public smartScreenSDKInterfaces::MessageListenerInterface
        , public alexaClientSDK::registrationManager::RegistrationObserverInterface
        , public smartScreenSDKInterfaces::MessagingServerObserverInterface
        , public alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface
        , public alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface
        , public alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface
        , public alexaClientSDK::avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<GUIClient> {
public:
    /// Alias for GUI provided token.
    using APLToken = uint64_t;

    /**
     * Create a @c GUIClient
     *
     * @param serverImplementation An implementation of @c MessagingInterface
     * @param miscStorage An implementation of MiscStorageInterface
     * @note The @c serverImplementation should implement the @c start method in a blocking fashion.
     * @return an instance of GUIClient.
     */
    static std::shared_ptr<GUIClient> create(
        std::shared_ptr<MessagingServerInterface> serverImplementation,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage);

    // @name TemplateRuntimeObserverInterface Functions
    /// @{
    void renderTemplateCard(const std::string& jsonPayload, alexaClientSDK::avsCommon::avs::FocusState focusState) override;
    void clearTemplateCard() override;
    void renderPlayerInfoCard(
        const std::string& jsonPayload,
        smartScreenSDKInterfaces::AudioPlayerInfo info,
        alexaClientSDK::avsCommon::avs::FocusState focusState) override;
    void clearPlayerInfoCard() override;
    /// @}

    // @name AlexaPresentationObserverInterface Functions
    /// @{
    void interruptCommandSequence() override;
    void renderDocument(const std::string& jsonPayload, const std::string& token, const std::string& windowId) override;
    void clearDocument() override;
    void executeCommands(const std::string& jsonPayload, const std::string& token) override;
    /// @}

    // @name VisualStateProviderInterface Functions
    /// @{
    void provideState(const unsigned int stateRequestToken) override;
    /// @}

    /// @name GUIClientInterface Functions
    /// @{
    void setGUIManager(
        std::shared_ptr<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIServerInterface> guiManager) override;
    bool acquireFocus(
        std::string channelName,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;
    bool releaseFocus(
        std::string channelName,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) override;
    void sendMessage(smartScreenSDKInterfaces::MessageInterface& message) override;
    /// @}

    /// @name MessagingServerInterface Functions
    /// @{
    bool start() override;
    void writeMessage(const std::string& payload) override;
    void setMessageListener(std::shared_ptr<MessageListenerInterface> messageListener) override;
    void stop() override;
    bool isReady() override;
    void setObserver(
        const std::shared_ptr<smartScreenSDKInterfaces::MessagingServerObserverInterface>& observer) override;
    /// @}

    /// @name MessagingServerObserverInterface Function
    /// @{
    void onConnectionOpened() override;
    void onConnectionClosed() override;
    /// @}

    /// @name MessageListenerInterface Function
    /// @{
    void onMessage(const std::string& jsonPayload) override;
    /// @}

    /// @name AuthObserverInterface Function
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
    /// @}

    /// @name CapabilitiesObserverInterface Function
    /// @{
    void onCapabilitiesStateChange(
        CapabilitiesObserverInterface::State newState,
        CapabilitiesObserverInterface::Error newError) override;
    /// @}

    /**
     * Processes user input until a quit command or a device reset is triggered.
     *
     * @return Returns a @c AVSClientRC.
     */
    alexaClientSDK::sampleApp::AVSClientRC run();

    /// @name RegistrationObserverInterface Functions
    /// @{
    void onLogout() override;
    /// @}

    /*
     * @return Returns the max APL version supported.
     */
    std::string getMaxAPLVersion();

    void setAplCoreConnectionManager(std::shared_ptr<AplCoreConnectionManager> aplCoreConnectionManager);

    void setAplCoreGuiRenderer(std::shared_ptr<AplCoreGuiRenderer> aplCoreGuiRenderer);

private:
    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

    /**
     * This class represents requestors as clients of FocusManager and handle notifications.
     */
    class ProxyFocusObserver : public alexaClientSDK::avsCommon::sdkInterfaces::ChannelObserverInterface {
    public:
        /**
         * Constructor.
         *
         * @param token Requester token.
         * @param focusBridge FocusBridge to perform operations through.
         * @param channelName Name of related channel.
         */
        ProxyFocusObserver(
            const APLToken token,
            std::shared_ptr<GUIClient> focusBridge,
            const std::string& channelName);

        /// @name ChannelObserverInterface Functions
        /// @{
        void onFocusChanged(alexaClientSDK::avsCommon::avs::FocusState newFocus) override;
        ///@}

    private:
        /// Related requester token.
        APLToken m_token;

        /// Parent FocusBridge.
        std::shared_ptr<GUIClient> m_focusBridge;

        /// Focus channel name.
        const std::string m_channelName;
    };

    /**
     * Constructor
     */
    GUIClient(
        std::shared_ptr<MessagingServerInterface> serverImplementation,
        const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
        const std::string& APLMaxVersion);

    /// Server worker thread.
    void serverThread();

    /// Send initRequest message to the client and wait for init response.
    void sendInitRequestAndWait();

    /// Process initResponse message received from the client.
    bool processInitResponse(const rapidjson::Document& message);

    /// Send viewport characteristic and GUI app config data in structured JSON format.
    void sendGuiConfiguration();

    /**
     * Handle TapToTalk event.
     *
     * @param message A complete message object with header and payload.
     */
    void handleTapToTalk(rapidjson::Document& message);

    /**
     * Handle HoldToTalk event.
     *
     * @param message A complete message object with header and payload.
     */
    void handleHoldToTalk(rapidjson::Document& message);

    /**
     * Handle focus acquire requests.
     *
     * @param message A complete message holding the request.
     */
    void handleFocusAcquireRequest(rapidjson::Document& message);

    /**
     * Handle focus release requests.
     *
     * @param message A complete message holding the request.
     */
    void handleFocusReleaseRequest(rapidjson::Document& message);

    /**
     * Handle focus message received confirmation messages.
     *
     * @param message A complete message holding the confirmation.
     */
    void handleOnFocusChangedReceivedConfirmation(rapidjson::Document& message);

    /**
     * Handle RenderStaticDocument message.
     *
     * @param message A complete message holding the render document directive.
     */
    void handleRenderStaticDocument(rapidjson::Document& message);

    /**
     * Handle ExecuteCommands message.
     *
     * @param message A complete message holding the confirmation.
     */
    void handleExecuteCommands(rapidjson::Document& message);

    /**
     * Handle activityEvent message.
     *
     * @param message A complete message holding the event.
     */
    void handleActivityEvent(rapidjson::Document& message);

    /**
     * Handle navigationEvent message.
     *
     * @param message A complete message holding the event.
     */
    void handleNavigationEvent(rapidjson::Document& message);

    /**
     * Handle logEvent message.
     *
     * @param message A complete message holding the event.
     */
    void handleLogEvent(rapidjson::Document& message);

    /**
     * Handle aplEvent message.
     *
     * @param message A complete message holding the event.
     */
    void handleAplEvent(rapidjson::Document& message);

    /**
     * Handle deviceWindowState message.
     * @param message A complete message holding the event.
     */
    void handleDeviceWindowState(rapidjson::Document& message);

    /// Internal function to execute @see processFocusAcquireRequest
    void executeFocusAcquireRequest(
        const APLToken token,
        const std::string& channelName,
        const std::string& avsInterface);

    /// Internal function to execute @see processFocusReleaseRequest
    void executeFocusReleaseRequest(const APLToken token, const std::string& channelName);

    /**
     * Send focus response.
     *
     * @param token Requester token.
     * @param result Result of focus operation.
     */
    void sendFocusResponse(const APLToken token, const bool result);

    /**
     * Starting timer to release channel in situations when focus operation result or
     * onFocusChanged event was not received by GUI so it will not know if it needs to release it.
     *
     * @param token The APL token.
     * @param channelName The channel to release.
     */
    void startAutoreleaseTimer(const APLToken token, const std::string& channelName);

    /**
     * Handle autoRelease.
     * @param token Requester token.
     * @param channelName Channel name.
     */
    void autoRelease(const APLToken token, const std::string& channelName);

    /**
     * Send focus change event notification.
     *
     * @param token Requester token.
     * @param state Resulting channel state.
     */
    void sendOnFocusChanged(const APLToken token, const alexaClientSDK::avsCommon::avs::FocusState state);

    /**
     * Process FocusManager focus acquire request from APL.
     *
     * @param token Operation token - unique per acquire/release request pair.
     * @param channelName Name of channel to acquire.
     * @param avsInterface AVS interface to report as owner of channel.
     */
    void processFocusAcquireRequest(
        const APLToken token,
        const std::string& channelName,
        const std::string& avsInterface);

    /**
     * Process FocusManager focus release request from APL.
     *
     * @param token Operation token - unique per acquire/release request pair.
     * @param channelName Name of channel to release.
     */
    void processFocusReleaseRequest(const APLToken token, const std::string& channelName);

    // The GUI manager implementation.
    std::shared_ptr<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIServerInterface> m_guiManager;

    /// An internal executor that performs execution of callable objects passed to it sequentially but asynchronously.
    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;

    // The server implementation.
    std::shared_ptr<smartScreenSDKInterfaces::MessagingServerInterface> m_serverImplementation;

    /// The thread used by the underlying server
    std::thread m_serverThread;

    // The thread used for init messages.
    std::thread m_initThread;

    /// Synchronize access between threads.
    std::mutex m_mutex;

    /// condition variable notify server state changed
    std::condition_variable m_cond;

    /// Has the underlying server started.
    std::atomic_bool m_hasServerStarted;

    /// Has initialization message from been received.
    std::atomic_bool m_initMessageReceived;

    /// Is the server in unrecoverable error state.
    std::atomic_bool m_errorState;

    /// The Listener to receive the messages
    std::shared_ptr<smartScreenSDKInterfaces::MessageListenerInterface> m_messageListener;

    /// APL max supported version
    std::string m_APLMaxVersion;

    /// Has the user logged out.
    std::atomic_bool m_shouldRestart;

    /// Persistent storage handle.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::storage::MiscStorageInterface> m_miscStorage;

    /// Server observer
    std::shared_ptr<smartScreenSDKInterfaces::MessagingServerObserverInterface> m_observer;

    /// The APL Core bridge
    std::shared_ptr<AplCoreConnectionManager> m_aplCoreConnectionManager;

    // The APL Core renderer
    std::shared_ptr<AplCoreGuiRenderer> m_aplCoreGuiRenderer;

    /// Flag to indicate that a fatal failure occurred. In this case, customer can either reset the device or kill
    /// the app.
    std::atomic_bool m_limitedInteraction;

    /// Map from message type to handling function.
    std::map<std::string, std::function<void(rapidjson::Document&)>> m_messageHandlers;

    /// Mutex for requester maps.
    std::mutex m_mapMutex;

    /// A map of APL side focus observers (proxies).
    std::map<APLToken, std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelObserverInterface>>
        m_focusObservers;

    /// Autorelease timers for case when client not received channel state change message.
    std::map<APLToken, std::shared_ptr<alexaClientSDK::avsCommon::utils::timing::Timer>> m_autoReleaseTimers;

    /// GUI log bridge to be used to handle log events.
    GUILogBridge m_rendererLogBridge;
};

}  // namespace gui
}  // namespace sampleApp
}  // namespace alexaSmartScreenSDK

#endif  // ALEXA_SMART_SCREEN_SDK_SAMPLEAPP_GUI_INCLUDE_SAMPLEAPP_GUI_GUICLIENT_H