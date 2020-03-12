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

#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>

#include <AVSCommon/Utils/AudioFormat.h>
#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <KWD/AbstractKeywordDetector.h>

#include "pryon_lite.h"

namespace alexaClient {
namespace kwd {


class PryonKeywordDetector : public alexaClientSDK::kwd::AbstractKeywordDetector {
public:
    /**
     * Creates a @c PryonKeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param modelFilePath The path to the model file.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Pryon at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events.
     * @return A new @c PryonKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<PryonKeywordDetector> create(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
        alexaClientSDK::avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        const std::string& modelFilePath,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Destructor.
     */
    ~PryonKeywordDetector() override;

private:
    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param modelFilePath The path to the model file.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Pryon at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events.
     */
    PryonKeywordDetector(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        alexaClientSDK::avsCommon::utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

    /**
     * Initializes the stream reader, sets up the engine, and kicks off a thread to begin processing data from
     * the stream. This function should only be called once with each new @c PryonKeywordDetector.
     *
     * @param modelFilePath The path to the model file.
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool initialize(const std::string& modelFilePath);

    /**
     * The detection loop feeds engine with data from the stream.
     *
     */
    void detectionLoop();

    /**
     * The detection callback fired when the keyword has been recognized.
     *
     * @param handle The PryonLiteDecoder handle
     * @param result The PryonLiteDecoder result object
     */
    static void detectionCallback(PryonLiteDecoderHandle handle, const PryonLiteResult* result);

    /**
     * The vad (voice-activity-detector) callback
     *
     * @param handle The PryonLiteDecoder handle
     * @param result The PryonLiteDecoder result object
     */
    static void vadCallback(PryonLiteDecoderHandle handle, const PryonLiteVadEvent* vadEvent);

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /// The stream of audio data.
    const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_stream;

    /// The reader that will be used to read audio data from the stream.
    std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Reader> m_streamReader;

    /// Internal thread that reads audio from the buffer and feeds it to the Sensory engine.
    std::thread m_detectionThread;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;

    /// The PryonLite decoder handle
    PryonLiteDecoderHandle m_decoder;

    /// The PryonLite configuration
    PryonLiteDecoderConfig m_config;

    /// The PryonLite current session info
    PryonLiteSessionInfo m_sessionInfo;

    /// The buffer for decoder
    char* m_decoderBuffer;

    /// The loaded model buffer
    uint8_t* m_modelBuffer;
};

}  // namespace kwd
}  // namespace alexaClientSDK
