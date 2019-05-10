// Copyright (c) 2014 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "nan.h"
#include "steam/steam_api.h"
#include "rail/sdk/rail_function_helper.h"
#include "v8.h"

#include "api/steam_api_registry.h"
#include "steam_client.h"
#include "steam_event.h"

HMODULE sdk_handle;
bool has_init = false;

namespace {

#define THROW_BAD_ARGS(msg)    \
    do {                       \
       Nan::ThrowTypeError(msg); \
       return;                   \
    } while (0);

#define MAKE_ENUM_PAIR(name) \
    { name, #name }

Nan::Persistent<v8::Object> g_persistent_steam_events;

NAN_METHOD(InitAPI) {
  Nan::HandleScope scope;

  bool success = SteamAPI_Init();

  if (success) {
    ISteamUserStats* steam_user_stats = SteamUserStats();
    steam_user_stats->RequestCurrentStats();
  }

  greenworks::SteamClient::GetInstance()->AddObserver(
      new greenworks::SteamEvent(g_persistent_steam_events));
  greenworks::SteamClient::StartSteamLoop();
  info.GetReturnValue().Set(Nan::New(success));
}


	HMODULE LoadWeGameSDKLibrary() {
		std::wstring rail_sdk_name;
#ifdef _WIN64  // windows for example
		rail_sdk_name = L".\\rail_api64.dll";
#else
		rail_sdk_name = L".\\rail_api.dll";
#endif

		std::wstring rail_sdk_path = rail_sdk_name;

        if (sdk_handle == NULL) {
            HMODULE dll_handle = ::LoadLibraryW(rail_sdk_path.c_str());
            if (NULL == dll_handle) {
                return NULL;
            }

            return dll_handle;
        }
	}

NAN_METHOD(initSdk) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  uint32 code = -1;

  if (!has_init) {
    sdk_handle = LoadWeGameSDKLibrary();
    if (sdk_handle != NULL) {
        rail::helper::Invoker invoker(sdk_handle);

        rail::RailGameID game_id = 2000958;
        char **argv;

        bool ret = invoker.RailNeedRestartAppForCheckingEnvironment(game_id, 0, const_cast<const char**>(argv));
        if (!ret) {
            ret = invoker.RailInitialize();
            if (ret) {
                ret = true;
                has_init = true;
                code = 0;
            }
        } else {
        	code = -3;
        }
    } else {
    	code = -2;
    }
  } else {
    ret = true;
    code = 0;
  }
  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  info.GetReturnValue().Set(result);
}

NAN_MODULE_INIT(init) {
  // Set internal steam event handler.
  v8::Local<v8::Object> steam_events = Nan::New<v8::Object>();
  g_persistent_steam_events.Reset(steam_events);
  Nan::Set(target, Nan::New("_steam_events").ToLocalChecked(), steam_events);

  greenworks::api::SteamAPIRegistry::GetInstance()->RegisterAllAPIs(target);

  Nan::Set(target,
           Nan::New("initAPI").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(InitAPI)->GetFunction());

  Nan::Set(target,
           Nan::New("initSdk").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(initSdk)->GetFunction());
}

}  // namespace

#if defined(_WIN32)
  #if defined(_M_IX86)
    NODE_MODULE(greenworks_win32, init)
  #elif defined(_M_AMD64)
    NODE_MODULE(greenworks_win64, init)
  #endif
#elif defined(__APPLE__)
  #if defined(__x86_64__) || defined(__ppc64__)
    NODE_MODULE(greenworks_osx64, init)
  #else
    NODE_MODULE(greenworks_osx32, init)
  #endif
#elif defined(__linux__)
  #if defined(__x86_64__) || defined(__ppc64__)
    NODE_MODULE(greenworks_linux64, init)
  #else
    NODE_MODULE(greenworks_linux32, init)
  #endif
#endif
