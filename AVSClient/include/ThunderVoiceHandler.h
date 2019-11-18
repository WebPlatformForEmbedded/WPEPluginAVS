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

#include <mutex>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <Audio/MicrophoneInterface.h>

#include "AVSDevice/InteractionManager.h"
#if defined(ENABLE_SMART_SCREEN_SUPPORT)
#include "SmartScreen/GUI/GUIManager.h"
#endif

#include "Module.h"
#include <WPEFramework/interfaces/IVoiceHandler.h>

namespace alexaClient {

/// String to identify log entries originating from this file.
static const std::string TAG("ThunderVoiceHandler");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)


/// Responsible for making an interaction on audio data
template<typename MANAGER>
class InteractionHandler {
public:
    /**
     * Creates a @c InteractionHandler.
     * After creation this class must be initialized with proper Interaction Manager
     *
     * @return A unique_ptr to a @c InteractionHandler if creation was successful and @c nullptr otherwise.
     */
    static std::unique_ptr<InteractionHandler> Create()
    {
        std::unique_ptr<InteractionHandler<MANAGER>> interactionHandler(new InteractionHandler<MANAGER>());
        if(!interactionHandler) {
            ACSDK_CRITICAL(LX("Failed to create a interaction handler!"));
            return nullptr;
        }

        return interactionHandler;
    }

    InteractionHandler(const InteractionHandler&) = delete;
    InteractionHandler& operator=(const InteractionHandler&) = delete;
    ~InteractionHandler() = default;

private:
    InteractionHandler()
        : m_interactionManager{nullptr} {}

public:
    bool Initialize(std::shared_ptr<MANAGER> interactionManager)
    {
        bool status = false;
        if(interactionManager) {
            m_interactionManager = interactionManager;
            status = true;
        }
        return status;
    }

    bool Deinitialize()
    {
        bool status = false;
        if(m_interactionManager) {
            m_interactionManager.reset();
            status = true;
        }
        return status;
    }

    void HoldToTalk();


private:
    std::shared_ptr<MANAGER> m_interactionManager;

};

template<>
inline void InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>::HoldToTalk()
{
    if(m_interactionManager) {
        m_interactionManager->holdToggled();
    }
}

template<>
inline void InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::HoldToTalk()
{
    if(m_interactionManager) {
        m_interactionManager->handleHoldToTalk();
    }
}

namespace AudioFormatCompatibility {
    /// The compatible AVS sample rate of 16 kHz.
    static constexpr unsigned int SAMPLE_RATE_HZ = 16000;
    /// The compatible bits per sample of 16.
    static constexpr unsigned int SAMPLE_SIZE_IN_BITS = 16;
    /// The compatible number of channels, which is 1.
    static constexpr unsigned int NUM_CHANNELS = 1;
    /// The compatible audio encoding of LPCM.
    static constexpr alexaClientSDK::avsCommon::utils::AudioFormat::Encoding ENCODING =
        alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;
    /// The compatible endianness which is little endian.
    static constexpr alexaClientSDK::avsCommon::utils::AudioFormat::Endianness ENDIANESS =
        alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;

    /**
     * Checks to see if an @c avsCommon::utils::AudioFormat is compatible.
     *
     * @param audioFormat The audio format to check.
     * @return @c true if the audio format is compatible and @c false otherwise.
     */
    static bool isCompatible(alexaClientSDK::avsCommon::utils::AudioFormat other) {
        if (ENCODING != other.encoding) {
            ACSDK_ERROR(LX("isAudioFormatCompatible")
                            .d("reason", "incompatibleEncoding")
                            .d("CompatibleEncoding", ENCODING)
                            .d("encoding", other.encoding));
            return false;
        }
        if (ENDIANESS != other.endianness) {
            ACSDK_ERROR(LX("isAudioFormatCompatible")
                            .d("reason", "incompatibleEndianess")
                            .d("CompatibleEndianness", ENDIANESS)
                            .d("endianness", other.endianness));
            return false;
        }
        if (SAMPLE_RATE_HZ != other.sampleRateHz) {
            ACSDK_ERROR(LX("isAudioFormatCompatible")
                            .d("reason", "incompatibleSampleRate")
                            .d("CompatibleSampleRate", SAMPLE_RATE_HZ)
                            .d("sampleRate", other.sampleRateHz));
            return false;
        }
        if (SAMPLE_SIZE_IN_BITS != other.sampleSizeInBits) {
            ACSDK_ERROR(LX("isAudioFormatCompatible")
                            .d("reason", "incompatibleSampleSizeInBits")
                            .d("CompatibleSampleSizeInBits", SAMPLE_SIZE_IN_BITS)
                            .d("sampleSizeInBits", other.sampleSizeInBits));
            return false;
        }
        if (NUM_CHANNELS != other.numChannels) {
            ACSDK_ERROR(LX("isAudioFormatCompatible")
                            .d("reason", "incompatibleNumChannels")
                            .d("CompatibleNumChannels", NUM_CHANNELS)
                            .d("numChannels", other.numChannels));
            return false;
        }

        return true;
    }
}; // namespace AudioFormatCompatibility

/// This class provides and audio input from Thunder
template<typename MANAGER>
class ThunderVoiceHandler : public alexaClientSDK::applicationUtilities::resources::audio::MicrophoneInterface {
public:
    /**
     * Creates a @c ThunderVoiceHandler.
     *
     * @param stream The shared data stream to write to.
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param callsign The callsign of nano service providing audio input.
     * @param interactionHandler The interaction handler that will execute necessary action like holdToTalk.
     * @param audioFormat The audioformat.
     * @return A unique_ptr to a @c ThunderVoiceHandler if creation was successful and @c nullptr otherwise.
     */
    static std::unique_ptr<ThunderVoiceHandler> create(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, WPEFramework::PluginHost::IShell *service, const string& callsign, std::shared_ptr<InteractionHandler<MANAGER>> interactionHandler, alexaClientSDK::avsCommon::utils::AudioFormat audioFormat)
    {
        if (!stream) {
            ACSDK_CRITICAL(LX("Invalid stream"));
            return nullptr;
        }

        if(!service) {
            ACSDK_CRITICAL(LX("Invalid service"));
            return nullptr;
        }

        if(!AudioFormatCompatibility::isCompatible(audioFormat)) {
            ACSDK_CRITICAL(LX("Audio Format is not compatible"));
            return nullptr;
        }

        std::unique_ptr<ThunderVoiceHandler> thunderVoiceHandler(new ThunderVoiceHandler(stream, service, callsign, interactionHandler));
        if(!thunderVoiceHandler) {
            ACSDK_CRITICAL(LX("Failed to create a ThunderVoiceHandler!"));
            return nullptr;
        }

        if(!thunderVoiceHandler->initialize()) {
            ACSDK_DEBUG0(LX("ThunderVoiceHandler is not initialized."));
        }

        return thunderVoiceHandler;
    }

    /**
     * Stops streaming from the microphone.
     *
     * @return Whether the stop was successful.
     */
    bool stopStreamingMicrophoneData() override
    {
        ACSDK_INFO(LX(__func__));

        return true;
    }

    /**
     * Starts streaming from the microphone.
     *
     * @return Whether the start was successful.
     */
    bool startStreamingMicrophoneData() override
    {
        ACSDK_INFO(LX(__func__));

        return true;
    }


    void stateChange(WPEFramework::PluginHost::IShell* audiosource)
    {
        if(audiosource->State() == WPEFramework::PluginHost::IShell::ACTIVATED) {
            bool status = initialize();
            if(!status) {
                ACSDK_CRITICAL(LX("Failed to initialize ThunderVoiceHandlerWraper"));
                return;
            }
        }

        if(audiosource->State() == WPEFramework::PluginHost::IShell::DEACTIVATED) {
            bool status = deinitialize();
            if(!status) {
                ACSDK_CRITICAL(LX("Failed to deinitialize ThunderVoiceHandlerWraper"));
                return;
            }
        }
    }

    /**
     * Destructor.
     */
    ~ThunderVoiceHandler() {}

private:
    /**
     * Constructor.
     *
     * @param stream The shared data stream to write to.
     * @param service The Thunder PluginHost::IShell of the nano service.
     * @param callsign The callsign of nano service providing audio input.
     * @param interactionHandler The interaction handler that will execute necessary action like holdToTalk.
     */
    ThunderVoiceHandler(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, WPEFramework::PluginHost::IShell *service, const string& callsign, std::shared_ptr<InteractionHandler<MANAGER>> interactionHandler)
        : m_audioInputStream{stream}
        , m_callsign{callsign}
        , m_service{service}
        , m_voiceProducer{nullptr}
        , m_voiceHandler{WPEFramework::Core::ProxyType<VoiceHandler>::Create(this)}
        , m_interactionHandler{interactionHandler}
        , m_isInitialized{false}
    {
    }

    /// Initializes ThunderVoiceHandler.
    bool initialize()
    {
        const std::lock_guard<std::mutex> lock{m_mutex};

        if(m_isInitialized) {
            return m_isInitialized;
        }

        m_writer = m_audioInputStream->createWriter(alexaClientSDK::avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
        if (!m_writer) {
            ACSDK_CRITICAL(LX("Failed to create stream writer"));
            return false;
        }

        m_voiceProducer = m_service->QueryInterfaceByCallsign<WPEFramework::Exchange::IVoiceProducer>(m_callsign);
        if(!m_voiceProducer) {
            ACSDK_ERROR(LX("Failed to obtain VoiceProducer interface!"));
            return false;
        }

        if(!m_voiceHandler) {
            ACSDK_ERROR(LX("Failed to obtain VoiceHandler!"));
            return false;
        }

        m_voiceProducer->Callback((&(*m_voiceHandler)));

        m_isInitialized = true;
        return m_isInitialized;
    }

    bool deinitialize()
    {
        const std::lock_guard<std::mutex> lock{m_mutex};

        if(m_writer) {
            m_writer.reset();
        }

        if(m_voiceProducer) {
            m_voiceProducer->Release();
        }

        m_isInitialized = false;
        return true;
    }

private:
    ///  Responsible for getting audio data from Thunder
    class VoiceHandler : public WPEFramework::Exchange::IVoiceHandler
    {
    public:
        /**
         * Contructor
         *
         * @param parent The parent class
         *
         */
        VoiceHandler(ThunderVoiceHandler *parent)
            : m_profile{nullptr}
            , m_parent{parent}
        {

        }

        /**
         * A callback method that is invoked with each start of the audio transmission.
         * The profile should stay the same between @c Start() and @c Stop().
         *
         * @param profile The audio profile of incoming data.
         */
        void Start(const WPEFramework::Exchange::IVoiceProducer::IProfile* profile) override
        {
            ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Start()"));

            m_profile = profile;
            if(m_profile) {
                m_profile->AddRef();
            }

            if(m_parent && m_parent->m_interactionHandler) {
                m_parent->m_interactionHandler->HoldToTalk();
            }
        }

        /**
         * A callback method that is invoked with each stop of the audio transmission.
         * The profile after @c Stop() is no longer valid.
         *
         */
        void Stop() override
        {
            ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Stop()"));

            if(m_profile){
                m_profile->Release();
                m_profile = nullptr;
            }

            if(m_parent && m_parent->m_interactionHandler) {
                m_parent->m_interactionHandler->HoldToTalk();
            }
        }

        /**
         * A callback method that is invoked with each data of the audio transmission.
         *
         * @param sequenceNo The sequence number of incoming data.
         * @param data The data itself.
         * @param length The length in bytes of audio data.
         */
        void Data(const uint32_t sequenceNo, const uint8_t data[], const uint16_t length) override
        {
            ACSDK_DEBUG0(LX("ThunderVoiceHandler::VoiceHandler::Data()"));

            if(m_parent && m_parent->m_writer) {
                // incoming data length = number of bytes
                size_t nWords = length / m_parent->m_writer->getWordSize();
                ssize_t rc = m_parent->m_writer->write(data, nWords);
                if(rc <= 0) {
                    ACSDK_CRITICAL(LX("Failed to write to stream.").d("rc", rc));
                }
            }
        }

        BEGIN_INTERFACE_MAP(VoiceHandler)
            INTERFACE_ENTRY(WPEFramework::Exchange::IVoiceHandler)
        END_INTERFACE_MAP

    private:
        /// Holds the profile between @c Start() and @c Stop().
        const WPEFramework::Exchange::IVoiceProducer::IProfile* m_profile;
        /// The parent class.
        ThunderVoiceHandler *m_parent;
    };

private:
    /// The stream of audio data.
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_audioInputStream;

    /// The writer that will be used to writer audio data into the sds.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Writer> m_writer;

    /**
     * A lock to seralize access to startStreamingMicrophoneData() and stopStreamingMicrophoneData() between different
     * threads.
     */
    std::mutex m_mutex;

    /// The callsing of the plugin providin voice audio input.
    string m_callsign;
    /// The Thunder PluginHost::IShell of the AVS plugin.
    WPEFramework::PluginHost::IShell* m_service;
    /// The Voice producer implementation
    WPEFramework::Exchange::IVoiceProducer* m_voiceProducer;
    /// The Voice Handler implementation
    WPEFramework::Core::ProxyType<VoiceHandler> m_voiceHandler;
    /// The interaction handler
    std::shared_ptr<InteractionHandler<MANAGER>> m_interactionHandler;
    /// The ThunderVoiceHandler initialization status
    bool m_isInitialized;

};

} // namespace alexaClient
