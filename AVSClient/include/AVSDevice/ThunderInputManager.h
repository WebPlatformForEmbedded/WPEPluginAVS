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

#include <atomic>

#include <WPEFramework/interfaces/IAVSClient.h>

#include "InteractionManager.h"
#include "AVSClientRC.h"

namespace alexaClientSDK {
namespace sampleApp {

    /// Observes user input from the console and notifies the interaction manager of the user's intentions.
class ThunderInputManager
    : public avsCommon::sdkInterfaces::AuthObserverInterface
    , public avsCommon::sdkInterfaces::CapabilitiesObserverInterface
    , public registrationManager::RegistrationObserverInterface
    , public avsCommon::sdkInterfaces::DialogUXStateObserverInterface {
public:
    /**
     * Creates a ThunderInputManager.
     *
     * @param interactionManager An instance of the @c InteractionManager used to manage user input.
     * @return Returns a new @c ThunderInputManager, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<ThunderInputManager> create(std::shared_ptr<InteractionManager> interactionManager);

    class AVSController : public WPEFramework::Exchange::IAVSController {
    public:
        AVSController(const AVSController&) = delete;
        AVSController& operator=(const AVSController&) = delete;
        ~AVSController() = default;

        /**
         * Constructor
         */
        AVSController(ThunderInputManager* parent);

        /**
         * Notifies registered clients about DialogUXState change
         * @param newState a new state
         */
        void NotifyDialogUXStateChanged(DialogUXState newState);

        /// @name WPEFramework::Exchange::IAVSController Functions
        /// @{
        void Register(INotification* sink) override;
        void Unregister(INotification* sink) override;
        uint32_t Mute(const bool mute) override;
        uint32_t Record(const bool start) override;
        /// @}

        BEGIN_INTERFACE_MAP(AVSController)
            INTERFACE_ENTRY(WPEFramework::Exchange::IAVSController)
        END_INTERFACE_MAP

    private:
        ThunderInputManager& m_parent;
        std::vector<WPEFramework::Exchange::IAVSController::INotification*> m_notifications;
    };


    /// @name RegistrationObserverInterface Functions
    /// @{
    void onLogout() override;
    /// @}

    /// @name DialogUXStateObserverInterface Functions
    /// @{
    void onDialogUXStateChanged(DialogUXState newState) override;
    /// @}

    WPEFramework::Exchange::IAVSController* Controller();

private:
    /**
     * Constructor
     */
    ThunderInputManager(std::shared_ptr<InteractionManager> interactionManager);

    std::atomic_bool m_limitedInteraction;

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

    std::shared_ptr<InteractionManager> m_interactionManager;
    WPEFramework::Core::ProxyType<AVSController> m_controller;

};

} // namespace sampleApp
} // namespace alexaClientSDK
