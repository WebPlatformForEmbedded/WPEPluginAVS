 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "ThunderVoiceHandler.h"

#include <WPEFramework/interfaces/IAVSClient.h>

#include <AVS/KWD/AbstractKeywordDetector.h>

#include <SampleApp/SampleApplication.h>

#include <vector>

namespace WPEFramework {
namespace Plugin {

    class SmartScreen
        : public WPEFramework::Exchange::IAVSClient,
          private alexaSmartScreenSDK::sampleApp::SampleApplication {
    public:
        SmartScreen()
            : _service(nullptr)
            , m_thunderVoiceHandler(nullptr)
        {
        }

        SmartScreen(const SmartScreen&) = delete;
        SmartScreen& operator=(const SmartScreen&) = delete;
        ~SmartScreen() {}

    private:
        class Config : public WPEFramework::Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : WPEFramework::Core::JSON::Container()
                , Audiosource()
                , AlexaClientConfig()
                , SmartScreenConfig()
                , LogLevel()
                , KWDModelsPath()
                , EnableKWD()
            {
                Add(_T("audiosource"), &Audiosource);
                Add(_T("alexaclientconfig"), &AlexaClientConfig);
                Add(_T("smartscreenconfig"), &SmartScreenConfig);
                Add(_T("loglevel"), &LogLevel);
                Add(_T("kwdmodelspath"), &KWDModelsPath);
                Add(_T("enablekwd"), &EnableKWD);
            }

            ~Config() = default;

        public:
            WPEFramework::Core::JSON::String Audiosource;
            WPEFramework::Core::JSON::String AlexaClientConfig;
            WPEFramework::Core::JSON::String SmartScreenConfig;
            WPEFramework::Core::JSON::String LogLevel;
            WPEFramework::Core::JSON::String KWDModelsPath;
            WPEFramework::Core::JSON::Boolean EnableKWD;
        };

    public:
        bool Initialize(PluginHost::IShell* service, const string& configuration) override;
        bool Deinitialize() override;
        Exchange::IAVSController* Controller() override;
        void StateChange(PluginHost::IShell* audioSource) override;

        BEGIN_INTERFACE_MAP(SmartScreen)
        INTERFACE_ENTRY(WPEFramework::Exchange::IAVSClient)
        END_INTERFACE_MAP

    private:
        bool Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder);
        bool InitSDKLogs(const string& logLevel);
        bool JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile);

    private:
        WPEFramework::PluginHost::IShell* _service;
        std::shared_ptr<ThunderVoiceHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>> m_thunderVoiceHandler;
#if defined(KWD_PRYON)
        std::unique_ptr<alexaClientSDK::kwd::AbstractKeywordDetector> m_keywordDetector;
#endif
    };

}
}
