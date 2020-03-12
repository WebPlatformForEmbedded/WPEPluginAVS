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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    ASSERT(service->PersistentPath() != _T(""));
    Core::Directory directory((service->PersistentPath() + _T("/db")).c_str());
    if (directory.CreatePath() != true) {
        message = _T("Failed to create Persistent Path");
    }

    if(message.empty() == true) {
        config.FromString(service->ConfigLine());
        if (config.AlexaClientConfig.IsSet() != true) {
            message = _T("Missing AlexaClient config file");
        }
    }

    if(message.empty() == true) {
        if (config.LogLevel.IsSet() != true) {
            message = _T("Missing log level");
        }
    }

    if(message.empty() == true) {
        _audiosourceName = config.Audiosource.Value();
        if(_audiosourceName.empty() == true) {
            message = _T("Missign audiosource callsing");
        }
    }

    if(message.empty() == true) {
        if(config.EnableSmartScreen.Value() == true) {
#if !defined(ENABLE_SMART_SCREEN_SUPPORT)
            message = _T("Smart Screen support is not compiled in!");
#else
            TRACE_L1(_T("Launching AVSClient - Smart Screen..."));

            _AVSClient = service->Root<Exchange::IAVSClient>(_connectionId, ImplWaitTime, _T("SmartScreen"));
            if(_AVSClient == nullptr) {
                message = _T("Failed to create the AVSClient - Smart Screen");
            } else {
                bool status = _AVSClient->Initialize(
                    service,
                    config.AlexaClientConfig.Value(),
                    config.SmartScreenConfig.Value(),
                    config.KWDModelsPath.Value(),
                    _audiosourceName,
                    config.EnableKWD.Value(),
                    config.LogLevel.Value());

                if(status != true) {
                    _AVSClient->Release();
                    message = _T("Failed to initialize the AVSClient - Smart Screen");
                }
            }
#endif
        } else {
            TRACE_L1(_T("Launching AVSClient - AVS Device..."));

            _AVSClient = service->Root<Exchange::IAVSClient>(_connectionId, ImplWaitTime, _T("AVSDevice"));
            if(_AVSClient == nullptr) {
                message = _T("Failed to create the AVSClient - AVSDevice");
            } else {
                bool status = _AVSClient->Initialize(
                    service,
                    config.AlexaClientConfig.Value(),
                    std::string(),
                    config.KWDModelsPath.Value(),
                    _audiosourceName,
                    config.EnableKWD.Value(),
                    config.LogLevel.Value());

                if(status == false) {
                    _AVSClient->Release();
                    message = _T("Failed to initialize the AVSClient - AVSDevice");
                }
            }
        }
    }

    if(message.empty() == true) {
        _controller = _AVSClient->Controller();
        if(_controller != nullptr) {
            _controller->AddRef();
            _controller->Register(&_dialogueNotification);
            Exchange::JAVSController::Register(*this, _controller);
        }
    }

    if(message.empty() == true) {
        service->Register(&_audiosourceNotification);
        service->Register(&_connectionNotification);
        _service =  service;
        _service->AddRef();
    }

    return message;
}

void AVS::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);


    if(_AVSClient != nullptr) {
        TRACE_L1(_T("Deinitializing AVSClient..."));

        if(_controller != nullptr) {
            _controller->Unregister(&_dialogueNotification);
            _controller->Release();
            Exchange::JAVSController::Unregister(*this);
        }

        if(_AVSClient->Deinitialize() == false) {
            TRACE_L1(_T("AVSClient deinitialize failed!"));
        }
        _AVSClient->Release();
    }

    _service->Unregister(&_audiosourceNotification);
    _service->Unregister(&_connectionNotification);
    _service->Release();
    _service = nullptr;
}

string AVS::Information() const
{
    return (_T("Alexa Voice Service Client"));
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
