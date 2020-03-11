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

#pragma once

#include "Module.h"

#include <interfaces/IAVSClient.h>
#include <interfaces/JAVSController.h>

#include "AVSDevice/AVSDevice.h"
#include "AVSClientRC.h"

#if defined(ENABLE_SMART_SCREEN_SUPPORT)
#include "SmartScreen/SmartScreen.h"
#endif

namespace WPEFramework {
namespace Plugin {

    class AVS
        : public PluginHost::IPlugin,
          public PluginHost::JSONRPC {
    public:
        AVS(const AVS&) = delete;
        AVS& operator=(const AVS&) = delete;

    private:

        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            ConnectionNotification() = delete;

        public:
            explicit ConnectionNotification(AVS* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~ConnectionNotification() = default;

        public:
            virtual void Activated(RPC::IRemoteConnection* connection) { _parent.Activated(connection); }

            virtual void Deactivated(RPC::IRemoteConnection* connection) { _parent.Deactivated(connection); }

            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            AVS& _parent;
        };

        class AudiosourceNotification : public PluginHost::IPlugin::INotification
        {
        public:
            AudiosourceNotification(const AudiosourceNotification&) = delete;
            AudiosourceNotification& operator=(const AudiosourceNotification&) = delete;

        public:
            explicit AudiosourceNotification(AVS* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~AudiosourceNotification() = default;

            BEGIN_INTERFACE_MAP(AudiosourceNotification)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        public:
            void StateChange(WPEFramework::PluginHost::IShell* service) override
            {
                if(!service) {
                    TRACE_L1("Service is a nullptr!");
                    return;
                }

                if(service->Callsign() == _parent._audiosourceName) {
                    if(_parent._AVSClient) {
                        _parent._AVSClient->StateChange(service);
                    }
                }
            }

        private:
            AVS& _parent;
        };

        class DialogueNotification : public Exchange::IAVSController::INotification {
        public:
            DialogueNotification(const DialogueNotification&) = delete;
            DialogueNotification& operator=(const DialogueNotification&) = delete;
            ~DialogueNotification() = default;

            DialogueNotification(AVS* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            void DialogueStateChange(const Exchange::IAVSController::INotification::dialoguestate state) const override
            {
                Exchange::JAVSController::Event::DialogueStateChange(_parent, state);
            }

            BEGIN_INTERFACE_MAP(DialogueNotification)
                INTERFACE_ENTRY(Exchange::IAVSController::INotification)
            END_INTERFACE_MAP

        private:
            AVS& _parent;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Audiosource()
                , AlexaClientConfig()
                , SmartScreenConfig()
                , LogLevel()
                , KWDModelsPath()
                , EnableSmartScreen()
                , EnableKWD()
            {
                Add(_T("audiosource"), &Audiosource);
                Add(_T("alexaclientconfig"), &AlexaClientConfig);
                Add(_T("smartscreenconfig"), &SmartScreenConfig);
                Add(_T("loglevel"), &LogLevel);
                Add(_T("kwdmodelspath"), &KWDModelsPath);
                Add(_T("enablesmartscreen"), &EnableSmartScreen);
                Add(_T("enablekwd"), &EnableKWD);
            }

            ~Config() = default;

        public:
            Core::JSON::String Audiosource;
            Core::JSON::String AlexaClientConfig;
            Core::JSON::String SmartScreenConfig;
            Core::JSON::String LogLevel;
            Core::JSON::String KWDModelsPath;
            Core::JSON::Boolean EnableSmartScreen;
            Core::JSON::Boolean EnableKWD;
        };


    public:
        static constexpr uint32_t ImplWaitTime = 2000;

        AVS()
            : _AVSClient(nullptr)
            , _controller(nullptr)
            , _service(nullptr)
            , _audiosourceName()
            , _connectionId(0)
            , _audiosourceNotification(this)
            , _connectionNotification(this)
            , _dialogueNotification(this)
        {
        }

        virtual ~AVS()
        {
        }

        BEGIN_INTERFACE_MAP(AVS)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Activated(RPC::IRemoteConnection* connection);
        void Deactivated(RPC::IRemoteConnection* connection);

        Exchange::IAVSClient* _AVSClient;
        Exchange::IAVSController* _controller;
        PluginHost::IShell* _service;
        string _audiosourceName;
        uint32_t _connectionId;
        Core::Sink<AudiosourceNotification> _audiosourceNotification;
        Core::Sink<ConnectionNotification> _connectionNotification;
        Core::Sink<DialogueNotification> _dialogueNotification;

    };

} // namespace Plugin
} // namespace WPEFramework
