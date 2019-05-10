// Copyright (c) 2016, ENTROPY GAME GLOBAL LIMITED. All rights reserved.
// file: rail_purchase_store.cpp

#include <process.h>
#include <shlwapi.h>
#include "RailPurchaseStore.h"

extern HMODULE sdk_handle;

RailPurchaseStore::RailPurchaseStore() {
	if (sdk_handle != NULL) {
		rail::helper::Invoker invoker(sdk_handle);
		invoker.RailRegisterEvent(
			rail::kRailEventInGamePurchaseAllPurchasableProductsInfoReceived, this);
		invoker.RailRegisterEvent(
			rail::kRailEventInGamePurchasePurchaseProductsToAssetsResult, this);
		invoker.RailRegisterEvent(rail::kRailEventInGamePurchasePurchaseProductsResult, this);
		invoker.RailRegisterEvent(rail::kRailEventInGamePurchaseFinishOrderResult, this);
		Inital();
	}
}

bool RailPurchaseStore::Inital() {
	if (sdk_handle != NULL) {
		rail::helper::Invoker invoker(sdk_handle);
		rail::IRailFactory* rail_factory = invoker.RailFactory();
		if (rail_factory == NULL) {
			return false;
		}

		rail_in_game_purchase_ = rail_factory->RailInGamePurchase();
		if (rail_in_game_purchase_ == NULL) {
			return false;
		}
		return true;
	}
	return false;
}

void RailPurchaseStore::Update(uint64_t ms) {
	UNREFERENCED_PARAMETER(ms);
}

bool RailPurchaseStore::AsyncRequestAllPurchasableProducts() {
	if (rail_in_game_purchase_) {
		rail::RailResult result = rail_in_game_purchase_->AsyncRequestAllPurchasableProducts("");
		if (result == rail::kSuccess)
			return true;
	}
	return false;
}
bool RailPurchaseStore::GetProductInfo(rail::RailProductID product_id,
	rail::RailPurchaseProductInfo* product) {
	if (rail_in_game_purchase_ && product) {
		rail::RailResult result = rail_in_game_purchase_->GetProductInfo(product_id, product);
		if (result == rail::kSuccess)
			return true;
	}
	return false;
}

bool RailPurchaseStore::AsyncPurchaseProductsToAssets(
	const rail::RailArray<rail::RailProductItem>& cart_items,
	const std::string& user_data /*= ""*/) {
	if (rail_in_game_purchase_) {
		rail::RailResult result =
			rail_in_game_purchase_->AsyncPurchaseProductsToAssets(cart_items,
				user_data.c_str());
		if (result == rail::kSuccess)
			return true;
	}
	return false;
}

bool RailPurchaseStore::AsyncPurchaseProducts(
	const rail::RailArray<rail::RailProductItem>& cart_items,
	const std::string& user_data /*= ""*/) {
	if (rail_in_game_purchase_) {
		rail::RailResult result = rail_in_game_purchase_->AsyncPurchaseProducts(cart_items,
			user_data.c_str());
		if (result == rail::kSuccess)
			return true;
	}
	return false;
}
bool RailPurchaseStore::AsyncFinishOrder(const std::string& order_id) {
	if (rail_in_game_purchase_) {
		rail::RailResult result = rail_in_game_purchase_->AsyncFinishOrder(order_id.c_str(), "");
		if (result == rail::kSuccess)
			return true;
	}
	return false;
}

RailPurchaseStore::~RailPurchaseStore() {
	if (sdk_handle != NULL) {
		rail::helper::Invoker invoker(sdk_handle);
		invoker.RailUnregisterEvent(
			rail::kRailEventInGamePurchaseAllPurchasableProductsInfoReceived,
			this);
		invoker.RailUnregisterEvent(rail::kRailEventInGamePurchasePurchaseProductsResult, this);
		invoker.RailUnregisterEvent(rail::kRailEventInGamePurchaseFinishOrderResult, this);
		invoker.RailUnregisterEvent(rail::kRailEventInGamePurchasePurchaseProductsToAssetsResult,
			this);
	}
}

void RailPurchaseStore::OnRailInGamePurchaseRequestAllPurchasableProductsResponse(
	rail::rail_event::RailInGamePurchaseRequestAllPurchasableProductsResponse* res) {
	bool is_succ = false;
	Isolate * isolate = Isolate::GetCurrent();
	std::vector<PurchaseStoreProductInfo> product_info_list;
	Local<Array> productsInfo = Array::New(isolate);

	if (res && res->result == rail::kSuccess) {
		for (uint32_t i = 0; i < res->purchasable_products.size(); i++) {
			PurchaseStoreProductInfo info;
			rail::RailPurchaseProductInfo& res_product_info = res->purchasable_products[i];
			productsInfo->Set(i * 4, Number::New(isolate, res_product_info.product_id));
			productsInfo->Set(i * 4 + 1, String::NewFromUtf8(isolate, res_product_info.name.c_str()));
			productsInfo->Set(i * 4 + 2, Number::New(isolate, res_product_info.original_price));
			productsInfo->Set(i * 4 + 3, String::NewFromUtf8(isolate, res_product_info.currency_type.c_str()));
		}
		is_succ = true;
	}
	RunRequestAllPurchasableProductsCallback(is_succ, productsInfo);
}

void RailPurchaseStore::RunRequestAllPurchasableProductsCallback(bool is_succ, Local<Array> productsInfo) {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, asyncRequestAllPurchasableProductsCallback);
	
	const unsigned argc = 2;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, is_succ),
		productsInfo
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
		asyncRequestAllPurchasableProductsCallback.Reset();
	}
}

void RailPurchaseStore::OnRailInGamePurchasePurchaseProductsToAssetsResponse(
	rail::rail_event::RailInGamePurchasePurchaseProductsToAssetsResponse* res) {
//	std::set<RailPurchaseStoreEvent*> listeners = listeners_;
//	std::set<RailPurchaseStoreEvent*>::iterator iter = listeners.begin();
//	for (; iter != listeners.end(); ++iter) {
//		if (*iter)
//			(*iter)->OnPurchaseStorePurchasePaymentToAssetsResult(res);
//	}
}

void RailPurchaseStore::OnRailInGamePurchasePurchaseProductsResponse(
	rail::rail_event::RailInGamePurchasePurchaseProductsResponse* res) {
	bool is_succ = false;
	char orderId[200];
	Isolate * isolate = Isolate::GetCurrent();

	if (res && res->result == rail::kSuccess) {
		is_succ = true;
	}
	strcpy_s(orderId, strlen(res->user_data.c_str()) + 1, res->user_data.c_str());
	RunAsyncPurchaseProductsCallback(is_succ, orderId);
}

void RailPurchaseStore::RunAsyncPurchaseProductsCallback(bool is_succ, std::string orderId) {
	Isolate * isolate = Isolate::GetCurrent();
	Local<Function> cb = Local<Function>::New(isolate, asyncPurchaseProductsCallback);

	const unsigned argc = 2;
	Local <Value> argv[argc] = {
		Boolean::New(isolate, is_succ),
		String::NewFromUtf8(isolate, orderId.c_str())
	};

	if (!cb->IsUndefined() && !cb->IsNull() && isolate) {
		cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
		asyncPurchaseProductsCallback.Reset();
		AsyncFinishOrder(orderId);
	}
}

void RailPurchaseStore::OnRailInGamePurchaseFinishOrderResponse(
	rail::rail_event::RailInGamePurchaseFinishOrderResponse* res) {
//	std::set<RailPurchaseStoreEvent*> listeners = listeners_;
//	std::set<RailPurchaseStoreEvent*>::iterator iter = listeners.begin();
//	for (; iter != listeners.end(); ++iter) {
//		if (*iter)
//			(*iter)->OnPurchaseStoreFinishOrderResult(res);
//	}
}

void RailPurchaseStore::OnRailEvent(rail::RAIL_EVENT_ID event_id, rail::EventBase* param) {
	UNREFERENCED_PARAMETER(param);
	switch (event_id) {
	case rail::kRailEventInGamePurchaseAllPurchasableProductsInfoReceived:
		OnRailInGamePurchaseRequestAllPurchasableProductsResponse(
			static_cast<rail::rail_event::RailInGamePurchaseRequestAllPurchasableProductsResponse*>(param));
		break;
	case rail::kRailEventInGamePurchasePurchaseProductsToAssetsResult:
		OnRailInGamePurchasePurchaseProductsToAssetsResponse(
			static_cast<rail::rail_event::RailInGamePurchasePurchaseProductsToAssetsResponse*>(param));
		break;
	case rail::kRailEventInGamePurchasePurchaseProductsResult:
		OnRailInGamePurchasePurchaseProductsResponse(
			static_cast<rail::rail_event::RailInGamePurchasePurchaseProductsResponse*>(param));
		break;
	case rail::kRailEventInGamePurchaseFinishOrderResult:
		OnRailInGamePurchaseFinishOrderResponse(
			static_cast<rail::rail_event::RailInGamePurchaseFinishOrderResponse*>(param));
		break;
	default:
		break;
	}
}

void RailPurchaseStore::AddListener(RailPurchaseStoreEvent* listener) {
	listeners_.insert(listener);
}

void RailPurchaseStore::RemoveListener(RailPurchaseStoreEvent* listener) {
	std::set<RailPurchaseStoreEvent*>::iterator find = listeners_.find(listener);
	if (find != listeners_.end()) {
		listeners_.erase(find);
		return;
	}
}
