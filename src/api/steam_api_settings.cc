// Copyright (c) 2016 Greenheart Games Pty. Ltd. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <map>
#include <sstream>
#include <string.h>

#include "nan.h"
#include "steam/steam_api.h"
#include "v8.h"

#include "greenworks_async_workers.h"
#include "greenworks_utils.h"
#include "greenworks_version.h"
#include "steam_api_registry.h"
#include "steam_client.h"
#include "steam_event.h"
#include "steam_id.h"
#include "rail/sdk/rail_function_helper.h"
#include "rail/sdk/rail_storage.h"
#include "rail/sdk/rail_dlc.h"
#include "RailPurchaseStore.h"
#include "CustomEvents.h"

extern HMODULE sdk_handle;

namespace greenworks {
namespace api {
namespace {

RailPurchaseStore *iRailPurchaseStore;
CustomEvents *iCustomEvents;

#define MAKE_ENUM_PAIR(name) \
    { name, #name }

void FreeCallback(char* data, void* hint) {
  delete data;
}

v8::Local<v8::Object> GetSteamUserCountType(int type_id) {
  if (type_id > k_EAccountTypeMax) {
    Nan::ThrowTypeError("Bad argument");
    return Nan::New<v8::Object>();
  }
  auto type = static_cast<EAccountType>(type_id);
  std::map<EAccountType, std::string> account_types = {
    MAKE_ENUM_PAIR(k_EAccountTypeInvalid),
    MAKE_ENUM_PAIR(k_EAccountTypeIndividual),
    MAKE_ENUM_PAIR(k_EAccountTypeMultiseat),
    MAKE_ENUM_PAIR(k_EAccountTypeGameServer),
    MAKE_ENUM_PAIR(k_EAccountTypeAnonGameServer),
    MAKE_ENUM_PAIR(k_EAccountTypePending),
    MAKE_ENUM_PAIR(k_EAccountTypeContentServer),
    MAKE_ENUM_PAIR(k_EAccountTypeClan),
    MAKE_ENUM_PAIR(k_EAccountTypeChat),
    MAKE_ENUM_PAIR(k_EAccountTypeConsoleUser),
    MAKE_ENUM_PAIR(k_EAccountTypeAnonUser),
    MAKE_ENUM_PAIR(k_EAccountTypeMax)
  };
  std::string name = account_types[type];
  v8::Local<v8::Object> account_type = Nan::New<v8::Object>();
  account_type->Set(Nan::New("name").ToLocalChecked(),
                    Nan::New(name).ToLocalChecked());
  account_type->Set(Nan::New("value").ToLocalChecked(), Nan::New(type_id));
  return account_type;
}

NAN_METHOD(RestartAppIfNecessary) {
  Nan::HandleScope scope;

  if (info.Length() < 1) {
    Nan::ThrowTypeError("You must pass your app ID to RestartAppIfNecessary");
    return;
  }

  if (!info[0]->IsUint32()) {
    Nan::ThrowTypeError("Your app ID argument should be an integer");
    return;
  }

  uint32 arg0 = info[0]->Uint32Value();

  bool restarting = SteamAPI_RestartAppIfNecessary(arg0);
  info.GetReturnValue().Set(Nan::New(restarting));
}

NAN_METHOD(IsSteamRunning) {
  Nan::HandleScope scope;

  bool running = SteamAPI_IsSteamRunning();
  info.GetReturnValue().Set(Nan::New(running));
}

NAN_METHOD(GetSteamId) {
  Nan::HandleScope scope;
  CSteamID user_id = SteamUser()->GetSteamID();
  v8::Local<v8::Object> flags = Nan::New<v8::Object>();
  flags->Set(Nan::New("anonymous").ToLocalChecked(),
             Nan::New(user_id.BAnonAccount()));
  flags->Set(Nan::New("anonymousGameServer").ToLocalChecked(),
             Nan::New(user_id.BAnonGameServerAccount()));
  flags->Set(Nan::New("anonymousGameServerLogin").ToLocalChecked(),
             Nan::New(user_id.BBlankAnonAccount()));
  flags->Set(Nan::New("anonymousUser").ToLocalChecked(),
             Nan::New(user_id.BAnonUserAccount()));
  flags->Set(Nan::New("chat").ToLocalChecked(),
             Nan::New(user_id.BChatAccount()));
  flags->Set(Nan::New("clan").ToLocalChecked(),
             Nan::New(user_id.BClanAccount()));
  flags->Set(Nan::New("consoleUser").ToLocalChecked(),
             Nan::New(user_id.BConsoleUserAccount()));
  flags->Set(Nan::New("contentServer").ToLocalChecked(),
             Nan::New(user_id.BContentServerAccount()));
  flags->Set(Nan::New("gameServer").ToLocalChecked(),
             Nan::New(user_id.BGameServerAccount()));
  flags->Set(Nan::New("individual").ToLocalChecked(),
             Nan::New(user_id.BIndividualAccount()));
  flags->Set(Nan::New("gameServerPersistent").ToLocalChecked(),
             Nan::New(user_id.BPersistentGameServerAccount()));
  flags->Set(Nan::New("lobby").ToLocalChecked(), Nan::New(user_id.IsLobby()));

  v8::Local<v8::Object> result = greenworks::SteamID::Create(user_id);
  // For backwards compatiblilty.
  result->Set(Nan::New("flags").ToLocalChecked(), flags);
  result->Set(Nan::New("type").ToLocalChecked(),
              GetSteamUserCountType(user_id.GetEAccountType()));
  result->Set(Nan::New("accountId").ToLocalChecked(),
              Nan::New<v8::Integer>(user_id.GetAccountID()));
  result->Set(Nan::New("steamId").ToLocalChecked(),
      Nan::New(utils::uint64ToString(
          user_id.ConvertToUint64())).ToLocalChecked());
  result->Set(Nan::New("staticAccountId").ToLocalChecked(),
      Nan::New(utils::uint64ToString(
          user_id.GetStaticAccountKey())).ToLocalChecked());
  result->Set(Nan::New("isValid").ToLocalChecked(),
              Nan::New<v8::Integer>(user_id.IsValid()));
  result->Set(Nan::New("level").ToLocalChecked(),
              Nan::New<v8::Integer>(SteamUser()->GetPlayerSteamLevel()));

  if (!SteamFriends()->RequestUserInformation(user_id, true)) {
    result->Set(Nan::New("screenName").ToLocalChecked(),
                Nan::New(SteamFriends()->GetFriendPersonaName(user_id))
                    .ToLocalChecked());
  } else {
    std::ostringstream sout;
    sout << user_id.GetAccountID();
    result->Set(Nan::New("screenName").ToLocalChecked(),
                Nan::New(sout.str()).ToLocalChecked());
  }
  info.GetReturnValue().Set(result);
}

NAN_METHOD(GetAppId) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(
      Nan::New(SteamUtils()->GetAppID()));
}

NAN_METHOD(GetCurrentGameLanguage) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(
      Nan::New(SteamApps()->GetCurrentGameLanguage()).ToLocalChecked());
}

NAN_METHOD(GetCurrentUILanguage) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(
      Nan::New(SteamUtils()->GetSteamUILanguage()).ToLocalChecked());
}

// TODO(hokein): Implement get game install directory.
NAN_METHOD(GetCurrentGameInstallDir) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(Nan::New("NOT IMPLEMENTED").ToLocalChecked());
}

NAN_METHOD(GetNumberOfPlayers) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    THROW_BAD_ARGS("Bad arguments");
  }
  Nan::Callback* success_callback =
      new Nan::Callback(info[0].As<v8::Function>());
  Nan::Callback* error_callback = nullptr;

  if (info.Length() > 1 && info[1]->IsFunction())
    error_callback = new Nan::Callback(info[1].As<v8::Function>());

  Nan::AsyncQueueWorker(new greenworks::GetNumberOfPlayersWorker(
      success_callback, error_callback));
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(IsGameOverlayEnabled) {
  Nan::HandleScope scope;
  info.GetReturnValue().Set(Nan::New(SteamUtils()->IsOverlayEnabled()));
}

NAN_METHOD(ActivateGameOverlay) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || !info[0]->IsString()) {
    THROW_BAD_ARGS("Bad arguments");
  }
  std::string option(*(v8::String::Utf8Value(info[0])));
  SteamFriends()->ActivateGameOverlay(option.c_str());
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(ActivateGameOverlayToWebPage) {
  Nan::HandleScope scope;
  if (info.Length() < 1 || !info[0]->IsString()) {
    THROW_BAD_ARGS("bad arguments");
  }
  std::string url = *(v8::String::Utf8Value(info[0]));
  SteamFriends()->ActivateGameOverlayToWebPage(url.c_str());
  info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(IsSubscribedApp) {
  Nan::HandleScope scope;
  if (info.Length() < 1) {
    Nan::ThrowTypeError("You must pass an app ID to IsSubscribedApp");
    return;
  }

  if (!info[0]->IsUint32()) {
    Nan::ThrowTypeError("Your app ID argument should be an integer");
    return;
  }

  uint32 arg0 = info[0]->Uint32Value();

  bool subscribed = SteamApps()->BIsSubscribedApp(arg0);
  info.GetReturnValue().Set(Nan::New(subscribed));
}

NAN_METHOD(GetImageSize) {
  Nan::HandleScope scope;
  if (info.Length() < 1 && !info[0]->IsInt32()) {
    THROW_BAD_ARGS("Bad arguments");
  }
  int image_handle = info[0]->Int32Value();
  uint32 width = 0;
  uint32 height = 0;
  if (!SteamUtils()->GetImageSize(image_handle, &width, &height)) {
    THROW_BAD_ARGS("Fail to get image size");
  }
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  result->Set(Nan::New("width").ToLocalChecked(), Nan::New(width));
  result->Set(Nan::New("height").ToLocalChecked(), Nan::New(height));
  info.GetReturnValue().Set(result);
}

NAN_METHOD(GetImageRGBA) {
  Nan::HandleScope scope;
  if (info.Length() < 1 && !info[0]->IsInt32()) {
    THROW_BAD_ARGS("Bad arguments");
  }
  int image_handle = info[0]->Int32Value();
  uint32 width = 0;
  uint32 height = 0;
  if (!SteamUtils()->GetImageSize(image_handle, &width, &height)) {
    THROW_BAD_ARGS("Fail to get image size");
  }
  int buffer_size = 4 * width * height;
  auto* image_buffer = new char[buffer_size];
  if (!SteamUtils()->GetImageRGBA(image_handle,
                                  reinterpret_cast<uint8*>(image_buffer),
                                  buffer_size)) {
    THROW_BAD_ARGS("Fail to get image");
  }
  info.GetReturnValue().Set(
      Nan::NewBuffer(image_buffer, buffer_size, FreeCallback, nullptr)
          .ToLocalChecked());
}

NAN_METHOD(hello) {
  Nan::HandleScope scope;

  info.GetReturnValue().Set(Nan::New("world").ToLocalChecked());
}

NAN_METHOD(railFire) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;

  if (sdk_handle != NULL) {
    rail::helper::Invoker invoker(sdk_handle);
    invoker.RailFireEvents();
    ret = true;
    code = 0;
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  info.GetReturnValue().Set(result);
}

NAN_METHOD(checkLogin) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;

  if (sdk_handle != NULL) {
    rail::helper::Invoker invoker(sdk_handle);
    rail::IRailPlayer* player = invoker.RailFactory()->RailPlayer();
	bool has_logged_in = player->AlreadyLoggedIn();

	if (has_logged_in) {
		ret = true;
		code = 0;
	} else {
		code = -3;
	}
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  info.GetReturnValue().Set(result);
}

NAN_METHOD(getUserId) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;
  std::string uid = "";

  if (sdk_handle != NULL) {
    rail::helper::Invoker invoker(sdk_handle);
    rail::IRailPlayer* player = invoker.RailFactory()->RailPlayer();
	bool has_logged_in = player->AlreadyLoggedIn();

	if (has_logged_in) {
		rail::RailID user_id = player->GetRailID();
		// 可以将用户ID转换为64位整数
		uint64_t num_user_id = user_id.get_id();
		uid = std::to_string(num_user_id);
		code = 0;
		ret = true;
	} else {
		code = -3;
	}
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  result->Set(Nan::New("uid").ToLocalChecked(), Nan::New(uid).ToLocalChecked());
  info.GetReturnValue().Set(result);
}

NAN_METHOD(getStorage) {
  Nan::HandleScope scope;
  rail::RailString filename = "imprison";
  rail::RailResult rail_result;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;
  std::string data = "";

  if (sdk_handle != NULL) {
    rail::helper::Invoker invoker(sdk_handle);
    rail::IRailFile* file = invoker.RailFactory()->RailStorageHelper()->OpenFile(filename, &rail_result);

	if (rail_result == rail::kSuccess) {
		uint32_t file_size = file->GetSize();
		char* buff = new char[file_size + 1];

		file->Read(buff, file_size, &rail_result);

		if (rail_result == rail::kSuccess) {
			buff[file_size] = '\0';
			data = buff;
			code = 0;
			ret = true;
		}
		else {
			code = -4;
		}
	}
	else {
		file = invoker.RailFactory()->RailStorageHelper()->CreateFileA(filename, &rail_result);
		const char *buff1 = "test";
		file->Write(buff1, strlen(buff1) + 1, &rail_result);

		if (rail_result == rail::kSuccess) {
			code = 0;
			ret = true;
			data = "test";
		}
		else {
			code = -3;
		}
	}

	file->Close();
	file->Release();
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  result->Set(Nan::New("data").ToLocalChecked(), Nan::New(data).ToLocalChecked());
  info.GetReturnValue().Set(result);
}

NAN_METHOD(setStorage) {
  Nan::HandleScope scope;
  rail::RailString filename = "imprison";
  rail::RailResult rail_result;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;
  std::string data = "";

  if (info.Length() < 1 || !info[0]->IsString()) {
	THROW_BAD_ARGS("Bad arguments");
  }

  std::string data_to_store = (*(v8::String::Utf8Value(info[0])));

  if (sdk_handle != NULL) {
    rail::helper::Invoker invoker(sdk_handle);
    rail::IRailFile* file = invoker.RailFactory()->RailStorageHelper()->CreateFileA(filename, &rail_result);
	uint32_t len = data_to_store.length();

	file->Write(data_to_store.c_str(), len, &rail_result);

	if (rail_result == rail::kSuccess) {
		data = data_to_store;
		code = 0;
		ret = true;
	}
	else {
		code = -3;
	}
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  result->Set(Nan::New("data").ToLocalChecked(), Nan::New(data).ToLocalChecked());
  info.GetReturnValue().Set(result);
}

NAN_METHOD(getProductsInfo) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;

  if (info.Length() < 1 || (!info[0]->IsFunction())) {
	THROW_BAD_ARGS("Bad arguments");
  }

  if (sdk_handle != NULL) {
	rail::IRailInGamePurchase* rail_in_game_purchase_ = iRailPurchaseStore->rail_in_game_purchase_;

    if (rail_in_game_purchase_ == NULL) {
    	code = -3;
    } else {
//    		Local<Function> cb = info[0].As<v8::Function>();
//    		if (iRailPurchaseStore == NULL) {
//    		  iRailPurchaseStore = new RailPurchaseStore();
//    		}
//    		v8::Isolate *isolate = info.GetReturnValue().GetIsolate();
//
//    		iRailPurchaseStore->asyncRequestAllPurchasableProductsCallback.Reset(isolate, cb);
//    		rail::RailResult result = rail_in_game_purchase_->AsyncRequestAllPurchasableProducts("all");
//    		if (result == rail::kSuccess) {
//    			code = 1;
//    			ret = true;
//    		} else {
    			code = -4;
//    		}
    }
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  info.GetReturnValue().Set(result);
}

NAN_METHOD(payForProduct) {
  Nan::HandleScope scope;
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  bool ret = false;
  int code = -1;

  if (info.Length() < 3 || (!info[1]->IsString()) || (!info[0]->IsNumber()) || (!info[2]->IsFunction())) {
	THROW_BAD_ARGS("Bad arguments");
  }

  if (sdk_handle != NULL) {
	rail::IRailInGamePurchase* rail_in_game_purchase_ = iRailPurchaseStore->rail_in_game_purchase_;

    if (rail_in_game_purchase_ == NULL) {
    	code = -3;
    } else {
    	if (rail_in_game_purchase_) {
    	    rail::RailArray<rail::RailProductItem> productItems;
    		int64_t product_id = info[0].As<v8::Number>()->NumberValue();
    		std::string order_id = *(v8::String::Utf8Value(info[1]));
    		rail::RailProductItem productItem;
			productItem.product_id = product_id;
			productItem.quantity = 1;
			productItems.push_back(productItem);
			Local<Function> cb = info[2].As<v8::Function>();
    		if (iRailPurchaseStore == NULL) {
    		  iRailPurchaseStore = new RailPurchaseStore();
    		}
    		v8::Isolate *isolate = info.GetReturnValue().GetIsolate();

    		iRailPurchaseStore->asyncPurchaseProductsCallback.Reset(isolate, cb);
    		rail::RailResult result = rail_in_game_purchase_->AsyncPurchaseProducts(productItems, order_id.c_str());
    		if (result == rail::kSuccess) {
    			code = 0;
    			ret = true;
    		} else {
    			code = -5;
    		}
    	} else {
    		code = -4;
    	}
    }
  }
  else {
    code = -2;
  }

  result->Set(Nan::New("ret").ToLocalChecked(), Nan::New(ret));
  result->Set(Nan::New("code").ToLocalChecked(), Nan::New(code));
  info.GetReturnValue().Set(result);
}

void RegisterAPIs(v8::Local<v8::Object> exports) {
  Nan::Set(exports,
           Nan::New("_version").ToLocalChecked(),
           Nan::New(GREENWORKS_VERSION).ToLocalChecked());
  Nan::Set(
      exports, Nan::New("restartAppIfNecessary").ToLocalChecked(),
      Nan::New<v8::FunctionTemplate>(RestartAppIfNecessary)->GetFunction());
  Nan::Set(exports,
           Nan::New("isSteamRunning").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(IsSteamRunning)->GetFunction());
  Nan::Set(exports,
           Nan::New("getSteamId").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetSteamId)->GetFunction());
  Nan::Set(exports,
           Nan::New("getAppId").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetAppId)->GetFunction());
  Nan::Set(exports,
           Nan::New("getCurrentGameLanguage").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(
               GetCurrentGameLanguage)->GetFunction());
  Nan::Set(exports,
           Nan::New("getCurrentUILanguage").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetCurrentUILanguage)->GetFunction());
  Nan::Set(exports,
           Nan::New("getCurrentGameInstallDir").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(
               GetCurrentGameInstallDir)->GetFunction());
  Nan::Set(exports,
           Nan::New("getNumberOfPlayers").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetNumberOfPlayers)->GetFunction());
  Nan::Set(exports,
           Nan::New("isGameOverlayEnabled").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(IsGameOverlayEnabled)->GetFunction());
  Nan::Set(exports,
           Nan::New("activateGameOverlay").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(ActivateGameOverlay)->GetFunction());
  Nan::Set(exports,
           Nan::New("activateGameOverlayToWebPage").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(
               ActivateGameOverlayToWebPage)->GetFunction());
  Nan::Set(exports,
           Nan::New("isSubscribedApp").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(IsSubscribedApp)->GetFunction());
  Nan::Set(exports,
           Nan::New("getImageSize").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetImageSize)->GetFunction());
  Nan::Set(exports,
           Nan::New("getImageRGBA").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(GetImageRGBA)->GetFunction());
  Nan::Set(exports,
           Nan::New("hello").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(hello)->GetFunction());
  Nan::Set(exports,
           Nan::New("railFire").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(railFire)->GetFunction());
  Nan::Set(exports,
           Nan::New("checkLogin").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(checkLogin)->GetFunction());
  Nan::Set(exports,
           Nan::New("getUserId").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(getUserId)->GetFunction());
  Nan::Set(exports,
           Nan::New("getStorage").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(getStorage)->GetFunction());
  Nan::Set(exports,
           Nan::New("setStorage").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(setStorage)->GetFunction());
  Nan::Set(exports,
           Nan::New("getProductsInfo").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(getProductsInfo)->GetFunction());
  Nan::Set(exports,
           Nan::New("payForProduct").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(payForProduct)->GetFunction());
}

SteamAPIRegistry::Add X(RegisterAPIs);

}  // namespace
}  // namespace api
}  // namespace greenworks
