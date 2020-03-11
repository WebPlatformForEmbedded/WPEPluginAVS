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

#include "AVS.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(AVS, 1, 0);

const string AVS::Initialize(PluginHost::IShell* service)
{
    string message = EMPTY_STRING;
    Config config;

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);

    _service = service;
    _service->Register(&_connectionNotification);

    ASSERT(service->PersistentPath() != _T(""));

    Core::Directory directory((service->PersistentPath() + "/db").c_str());
    if (directory.CreatePath() != true) {
        message = "Failed to create Persistent Path";
        TRACE_L1(message);
        return message;
    }

    config.FromString(_service->ConfigLine());
    if (config.AlexaClientConfig.IsSet() == false) {
        message = "Missing AlexaClient config file";
        TRACE_L1(message);
        return message;
    }
    if (config.LogLevel.IsSet() == false) {
        message = "Missing log level";
        TRACE_L1(message);
        return message;
    }
    _audiosourceName = config.Audiosource.Value();
    if(_audiosourceName.empty() == true) {
        message = "Missign audiosource callsing";
        TRACE_L1(message);
        return message;
    }

    if(config.EnableSmartScreen.Value() == true) {
#if defined(ENABLE_SMART_SCREEN_SUPPORT)
        TRACE_L1("Launching AVSClient - Smart Screen...");

        _AVSClient = _service->Root<Exchange::IAVSClient>(_connectionId, ImplWaitTime, _T("SmartScreen"));
        if(_AVSClient == false) {
            message = "Failed to create the AVSClient - Smart Screen";
            return message;
        }

        bool status = _AVSClient->Initialize(
            _service,
            config.AlexaClientConfig.Value(),
            config.SmartScreenConfig.Value(),
            config.KWDModelsPath.Value(),
            _audiosourceName,
            config.EnableKWD.Value(),
            config.LogLevel.Value());

        if(status == false) {
            message = "Failed to initialize the AVSClient - Smart Screen";
            return message;
        }
#else
        message = "Smart Screen support is not compiled in!";
        TRACE_L1(message);
        return message;
#endif
    } else {
        TRACE_L1("Launching AVSClient - AVS Device...");

        _AVSClient = _service->Root<Exchange::IAVSClient>(_connectionId, ImplWaitTime, _T("AVSDevice"));
        if(_AVSClient == false) {
            message = "Failed to create the AVSClient - AVSDevice";
            return message;
        }

        bool status = _AVSClient->Initialize(
            _service,
            config.AlexaClientConfig.Value(),
            std::string(),
            config.KWDModelsPath.Value(),
            _audiosourceName,
            config.EnableKWD.Value(),
            config.LogLevel.Value());

        if(status == false) {
            message = "Failed to initialize the AVSClient - AVSDevice";
            return message;
        }
    }

    _controller = _AVSClient->Controller();
    if(_controller == false) {
        message = "Failed to obtain AVS Controller";
        return message;
    }
    _controller->Register(&_dialogueNotification);
    Exchange::JAVSController::Register(*this, _controller);

    _service->Register(&_audiosourceNotification);

    return message;
}

void AVS::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    Exchange::JAVSController::Unregister(*this);

    if(_AVSClient != false) {
        TRACE_L1("Deinitializing AVSClient...");
        if(_controller != false) {
            _controller->Unregister(&_dialogueNotification);
        }

        if(_AVSClient->Deinitialize() == false) {
            TRACE_L1("AVSClient deinitialize failed!");
        }
    }

    _service->Unregister(&_connectionNotification);
    _service->Unregister(&_audiosourceNotification);
    _service = nullptr;
}

string AVS::Information() const
{
    return ((_T("Alexa Voice Service Client")));
}

void AVS::Activated(RPC::IRemoteConnection* /*connection*/)
{
    return;
}

void AVS::Deactivated(RPC::IRemoteConnection* connection)
{
    if (_connectionId == connection->Id()) {
        ASSERT(_service != nullptr);
        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}
} // namespace Plugin
} // namespace WPEFramework
