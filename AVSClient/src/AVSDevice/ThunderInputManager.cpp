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

#include "ThunderInputManager.h"

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::sdkInterfaces;
using namespace WPEFramework::Exchange;

/// String to identify log entries originating from this file.
static const std::string TAG("ThunderInputManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<ThunderInputManager> ThunderInputManager::create(std::shared_ptr<InteractionManager> interactionManager) {
    if (!interactionManager) {
        ACSDK_CRITICAL(LX("Invalid InteractionManager passed to UserInputManager"));
        return nullptr;
    }
    return std::unique_ptr<ThunderInputManager>(new ThunderInputManager(interactionManager));
}

ThunderInputManager::ThunderInputManager(std::shared_ptr<InteractionManager> interactionManager)
    : m_limitedInteraction{false}
    , m_interactionManager{interactionManager}
    , m_controller{WPEFramework::Core::ProxyType<AVSController>::Create(this)} {
}

void ThunderInputManager::onDialogUXStateChanged(DialogUXState newState) {
    if(m_controller) {
        m_controller->NotifyDialogUXStateChanged(newState);
    }
}

ThunderInputManager::AVSController::AVSController(ThunderInputManager* parent)
    : m_parent(*parent)
    , m_notifications() {
    for(auto* notification : m_notifications) {
        notification->Release();
    }
    m_notifications.clear();
}

void ThunderInputManager::AVSController::NotifyDialogUXStateChanged(DialogUXState newState) {
    for (auto* notification : m_notifications) {
        notification->DialogueStateChange(static_cast<IAVSController::INotification::dialoguestate>(newState));
    }
}

void ThunderInputManager::AVSController::Register(INotification* notification) {
    ASSERT(notification != nullptr);
    notification->AddRef();
    m_notifications.push_back(notification);
}

void ThunderInputManager::AVSController::Unregister(INotification* notification) {
    ASSERT(notification != nullptr);

    auto item = std::find(m_notifications.begin(), m_notifications.end(), notification);
    if(item != m_notifications.end()) {
        m_notifications.erase(item);
        (*item)->Release();
    }
}

uint32_t ThunderInputManager::AVSController::Mute(const bool mute) {
    if(m_parent.m_limitedInteraction) {
        return static_cast<uint32_t>(WPEFramework::Core::ERROR_GENERAL);
    }

    if(!m_parent.m_interactionManager) {
        return static_cast<uint32_t>(WPEFramework::Core::ERROR_UNAVAILABLE);
    }

    m_parent.m_interactionManager->setMute(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME, mute);
    m_parent.m_interactionManager->setMute(avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME, mute);

    return static_cast<uint32_t>(WPEFramework::Core::ERROR_NONE);
}

uint32_t ThunderInputManager::AVSController::Record(const bool start) {
    if(m_parent.m_limitedInteraction) {
        return static_cast<uint32_t>(WPEFramework::Core::ERROR_GENERAL);
    }

    if(!m_parent.m_interactionManager) {
        return static_cast<uint32_t>(WPEFramework::Core::ERROR_UNAVAILABLE);
    }

    m_parent.m_interactionManager->tap();

    return static_cast<uint32_t>(WPEFramework::Core::ERROR_NONE);
}

WPEFramework::Exchange::IAVSController* ThunderInputManager::Controller() {
    return (&(*m_controller));
}

void ThunderInputManager::onLogout()
{
    m_limitedInteraction = true;
}

void ThunderInputManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError)
{
    m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
}

void ThunderInputManager::onCapabilitiesStateChange(
    CapabilitiesObserverInterface::State newState,
    CapabilitiesObserverInterface::Error newError)
{
    m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesObserverInterface::State::FATAL_ERROR);
}
} // namespace sampleApp
} // namespace alexaClientSDK
