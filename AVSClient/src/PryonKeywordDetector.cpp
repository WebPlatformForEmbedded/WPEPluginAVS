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
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "PryonKeywordDetector.h"

#include <memory>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>

#include "Module.h"

namespace alexaClient {
namespace kwd {

using namespace alexaClientSDK::avsCommon;
using namespace alexaClientSDK::avsCommon::avs;
using namespace alexaClientSDK::avsCommon::sdkInterfaces;
using namespace alexaClientSDK::avsCommon::utils::logger;

/// String to identify log entries originating from this file.
static const std::string TAG("PryonKeywordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The number of hertz per kilohertz.
static const size_t HERTZ_PER_KILOHERTZ = 1000;

/// The timeout to use for read calls to the SharedDataStream.
const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

/// The compatible AVS sample rate of 16 kHz.
static const unsigned int COMPATIBLE_SAMPLE_RATE = 16000;

/// The compatible bits per sample of 16.
static const unsigned int COMPATIBLE_SAMPLE_SIZE_IN_BITS = 16;

/// The compatible number of channels, which is 1.
static const unsigned int COMPATIBLE_NUM_CHANNELS = 1;

/// The compatible audio encoding of LPCM.
static const alexaClientSDK::avsCommon::utils::AudioFormat::Encoding COMPATIBLE_ENCODING =
    alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

/// The compatible endianness which is little endian.
static const alexaClientSDK::avsCommon::utils::AudioFormat::Endianness COMPATIBLE_ENDIANNESS =
    alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;

/// Locale value for choosing proper model for the detection engine
static const char* DEFAULT_LOCALE = "en-US";

/// The key storing the models for locale
static const std::string KEY_MODEL_LOCALES = "alexa";

/// The detection keyword
static constexpr const char* DETECTION_KEYWORD = "ALEXA";

/// The detection treshold between 0 and 1000; Less = more permissive (empiric)
static constexpr const uint32_t DETECTION_TRESHOLD = 200;

/**
 * Checks to see if an @c avsCommon::utils::AudioFormat is compatible.
 *
 * @param audioFormat The audio format to check.
 * @return @c true if the audio format is compatible and @c false otherwise.
 */
static bool isAudioFormatCompatible(alexaClientSDK::avsCommon::utils::AudioFormat audioFormat) {
    if (COMPATIBLE_ENCODING != audioFormat.encoding) {
        ACSDK_ERROR(LX("isAudioFormatCompatible")
                        .d("reason", "incompatibleEncoding")
                        .d("CompatibleEncoding", COMPATIBLE_ENCODING)
                        .d("encoding", audioFormat.encoding));
        return false;
    }
    if (COMPATIBLE_ENDIANNESS != audioFormat.endianness) {
        ACSDK_ERROR(LX("isAudioFormatCompatible")
                        .d("reason", "incompatibleEndianess")
                        .d("CompatibleEndianness", COMPATIBLE_ENDIANNESS)
                        .d("endianness", audioFormat.endianness));
        return false;
    }
    if (COMPATIBLE_SAMPLE_RATE != audioFormat.sampleRateHz) {
        ACSDK_ERROR(LX("isAudioFormatCompatible")
                        .d("reason", "incompatibleSampleRate")
                        .d("CompatibleSampleRate", COMPATIBLE_SAMPLE_RATE)
                        .d("sampleRate", audioFormat.sampleRateHz));
        return false;
    }
    if (COMPATIBLE_SAMPLE_SIZE_IN_BITS != audioFormat.sampleSizeInBits) {
        ACSDK_ERROR(LX("isAudioFormatCompatible")
                        .d("reason", "incompatibleSampleSizeInBits")
                        .d("CompatibleSampleSizeInBits", COMPATIBLE_SAMPLE_SIZE_IN_BITS)
                        .d("sampleSizeInBits", audioFormat.sampleSizeInBits));
        return false;
    }
    if (COMPATIBLE_NUM_CHANNELS != audioFormat.numChannels) {
        ACSDK_ERROR(LX("isAudioFormatCompatible")
                        .d("reason", "incompatibleNumChannels")
                        .d("CompatibleNumChannels", COMPATIBLE_NUM_CHANNELS)
                        .d("numChannels", audioFormat.numChannels));
        return false;
    }

    return true;
}

std::unique_ptr<PryonKeywordDetector> PryonKeywordDetector::create(
    std::shared_ptr<AudioInputStream> stream,
    utils::AudioFormat audioFormat,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>
        keyWordDetectorStateObservers,
    const std::string& modelsFilePath,
    std::chrono::milliseconds msToPushPerIteration) {
    if (!stream) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
        return nullptr;
    }

    if (!isAudioFormatCompatible(audioFormat)) {
        return nullptr;
    }

    std::unique_ptr<PryonKeywordDetector> detector(new PryonKeywordDetector(
        stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat, msToPushPerIteration));
    if (!detector->initialize(modelsFilePath)) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
        return nullptr;
    }

    return detector;
}

PryonKeywordDetector::~PryonKeywordDetector() {
    m_isShuttingDown = true;
    if (m_detectionThread.joinable()) {
        m_detectionThread.join();
    }

    PryonLiteError error = PryonLiteDecoder_Destroy(&m_decoder);
    if(error != PRYON_LITE_ERROR_OK) {
        ACSDK_ERROR(LX("~PryonKeywordDetectorFailed").d("error", error));
    }

    delete[] m_decoderBuffer;
    delete[] m_modelBuffer;
}

PryonKeywordDetector::PryonKeywordDetector(
    std::shared_ptr<AudioInputStream> stream,
    std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
    utils::AudioFormat audioFormat,
    std::chrono::milliseconds msToPushPerIteration) :
        AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
        m_isShuttingDown{false},
        m_stream{stream},
        m_streamReader{nullptr},
        m_detectionThread{},
        m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count()),
        m_decoder{nullptr},
        m_config{},
        m_sessionInfo{},
        m_decoderBuffer{nullptr},
        m_modelBuffer{nullptr} {
}

bool PryonKeywordDetector::initialize(const std::string& modelFilePath) {
    m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
    if (!m_streamReader) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "createStreamReaderFailed"));
        return false;
    }

    m_config = PryonLiteDecoderConfig_Default;

    std::set<std::string> localePaths;
    auto localeToModelsConfig = alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[KEY_MODEL_LOCALES];
    bool isLocaleFound = localeToModelsConfig.getStringValues(DEFAULT_LOCALE, &localePaths);
    if(!isLocaleFound) {
        ACSDK_ERROR(LX("initializeFailed()")
            .d("reason", "localeNotFound")
            .d("defaultLocale", DEFAULT_LOCALE)
        );
        return false;
    }

    std::string localizedModelFilepath = "";
    for(auto it = localePaths.cbegin(); it != localePaths.cend(); ++it) {
        if(!it->empty()) {
            localizedModelFilepath = modelFilePath + "/" + (*it) + ".bin";
        }
    }

    WPEFramework::Core::File modelFile(localizedModelFilepath);
    if(modelFile.Open()) {
        m_config.sizeofModel = modelFile.Size();
        m_modelBuffer = new uint8_t[m_config.sizeofModel]();
        uint32_t nRead = modelFile.Read(m_modelBuffer, m_config.sizeofModel);
        if(nRead != m_config.sizeofModel) {
            ACSDK_ERROR(LX("initializeFailed")
                .d("reason", "readModelFileFailed")
                .d("nRead", nRead)
                .d("m_config.sizeofModel", m_config.sizeofModel));
            return false;
        }
    } else {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "openModelFileFailed"));
        return false;
    }
    modelFile.Close();

    m_config.model = m_modelBuffer;

    // Query for the size of instance memory required by the decoder
    PryonLiteModelAttributes modelAttributes;
    PryonLiteError error = PryonLite_GetModelAttributes(m_config.model, m_config.sizeofModel, &modelAttributes);
    if(error) {
        ACSDK_ERROR(LX("initializeFailed")
            .d("reason", "GetModelAttributesFailed")
            .d("error", error));
            return false;
    }

    m_decoderBuffer = new char[modelAttributes.requiredDecoderMem]();
    m_config.decoderMem = m_decoderBuffer;
    m_config.sizeofDecoderMem = modelAttributes.requiredDecoderMem;
    m_config.userData = reinterpret_cast<void*>(this);
    m_config.detectThreshold = DETECTION_TRESHOLD;
    m_config.resultCallback = detectionCallback;
    m_config.vadCallback = vadCallback;
    m_config.useVad = 1;

    error = PryonLiteDecoder_Initialize(&m_config, &m_sessionInfo, &m_decoder);
    if(error) {
        ACSDK_ERROR(LX("initializeFailed")
            .d("reason", "PryonLiteDecoder_Initialize")
            .d("error", error));
            return false;
    }

    error = PryonLiteDecoder_SetDetectionThreshold(m_decoder, DETECTION_KEYWORD, m_config.detectThreshold);
    if(error) {
        ACSDK_ERROR(LX("initializeFailed")
            .d("reason", "PryonLiteDecoder_SetDetectionThresholdFailed")
            .d("error", error));
            return false;
    }

    m_isShuttingDown = false;
    m_detectionThread = std::thread(&PryonKeywordDetector::detectionLoop, this);
    return true;
}

void PryonKeywordDetector::detectionLoop()
{
    std::vector<int16_t> audioDataToPush(m_maxSamplesPerPush);
    ssize_t wordsRead;
    bool didErrorOccur = false;
    PryonLiteError writeStatus = PRYON_LITE_ERROR_OK;

    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

    while(!m_isShuttingDown) {
        wordsRead = readFromStream(
            m_streamReader,
            m_stream,
            audioDataToPush.data(),
            audioDataToPush.size(),
            TIMEOUT_FOR_READ_CALLS,
            &didErrorOccur);

        if(didErrorOccur) {
            ACSDK_ERROR(LX("detectionLoop").d("readFromStreamStatus", "OVERRUN"));
            break;
        } else if(wordsRead > 0) {
            writeStatus = PryonLiteDecoder_PushAudioSamples(m_decoder, audioDataToPush.data(), audioDataToPush.size());
            if(writeStatus) {
                ACSDK_ERROR(LX("detectionLoop")
                    .d("result", "PryonLiteDecoder_PushAudioSamplesFailure")
                    .d("error", writeStatus));
                notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                break;
            }
        } else {
            ACSDK_ERROR(LX("detectionLoop")
                .d("readFromStreamStatus", "unexpectedCase")
                .d("wordsRead", wordsRead));
        }
    }

    m_streamReader->close();
    ACSDK_DEBUG0(LX("detectionLoop").d("status", "detectionThreadEnd"));
}


/* static */ void PryonKeywordDetector::detectionCallback(PryonLiteDecoderHandle handle, const PryonLiteResult* result)
{
    ACSDK_DEBUG0(LX(__func__));

    if(!result) {
        ACSDK_ERROR(LX("detectionCallback").d("reason", "resultIsNullptr"));
        return;
    }

    const PryonKeywordDetector* pryonKWD = reinterpret_cast<PryonKeywordDetector*>(result->userData);
    if(!pryonKWD) {
        ACSDK_ERROR(LX("detectionCallback").d("reason", "userDataIsNullptr"));
        return;
    }

    auto sampleLen = result->endSampleIndex - result->beginSampleIndex;
    ACSDK_DEBUG0(LX("detectionCallbackResult")
                    .d("confidence", result->confidence)
                    .d("beginSampleIndex", result->beginSampleIndex)
                    .d("endSampleIndex", result->endSampleIndex)
                    .d("m_streamReader->tell()", pryonKWD->m_streamReader->tell())
                    .d("sampleLen", sampleLen)
                    .d("keyword", result->keyword));

    pryonKWD->notifyKeyWordObservers(
        pryonKWD->m_stream,
        result->keyword,
        pryonKWD->m_streamReader->tell() - sampleLen,
        pryonKWD->m_streamReader->tell());
}

/* static */ void PryonKeywordDetector::vadCallback(PryonLiteDecoderHandle handle, const PryonLiteVadEvent* vadEvent)
{
    ACSDK_DEBUG0(LX("vadCallback").d("vadState", vadEvent->vadState));
}

}  // namespace kwd
}  // namespace alexaClientSDK
