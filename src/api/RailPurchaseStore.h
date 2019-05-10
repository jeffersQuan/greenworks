// Copyright (c) 2016, ENTROPY GAME GLOBAL LIMITED. All rights reserved.
// file: rail_purchase_store.h

#ifndef SDK_WRAPPER_RAIL_PURCHASE_STORE_H
#define SDK_WRAPPER_RAIL_PURCHASE_STORE_H

#include <string>
#include <set>
#include <map>
#include <vector>
#include "rail/sdk/rail_api.h"
#include <node.h>

using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Persistent;
using v8::Null;
using v8::Object;
using v8::String;
using v8::Boolean;
using v8::Number;
using v8::Array;
using v8::Value;


struct PurchaseStoreProductInfo {
	rail::RailPurchaseProductInfo product_info;
	bool is_discount_price_valid;
	PurchaseStoreProductInfo() { is_discount_price_valid = false; }
};

typedef std::map<rail::RailProductID, PurchaseStoreProductInfo> ProductInfoMap;
typedef std::pair<rail::RailProductID, PurchaseStoreProductInfo> ProductInfoMapPair;

class RailPurchaseStoreEvent {
public:
	// Get Friends Personal Info
	virtual ~RailPurchaseStoreEvent() {}
	virtual void OnGetPurchaseStoreAllProducts(bool /*is_succ*/,
		const std::vector<PurchaseStoreProductInfo>& /*product_info_list*/) {}
	virtual void OnPurchaseStorePurchasePaymentToAssetsResult(
		rail::rail_event::RailInGamePurchasePurchaseProductsToAssetsResponse* /*res*/) {}
	virtual void OnPurchaseStorePurchasePaymentResult(
		rail::rail_event::RailInGamePurchasePurchaseProductsResponse* /*res*/) {}
	virtual void OnPurchaseStoreFinishOrderResult(
		rail::rail_event::RailInGamePurchaseFinishOrderResponse* /*res*/) {}
};

class RailPurchaseStore : public rail::IRailEvent {
public:
	RailPurchaseStore();
	~RailPurchaseStore();

	rail::IRailInGamePurchase* rail_in_game_purchase_;
	Persistent<Function> asyncRequestAllPurchasableProductsCallback;
	Persistent<Function> asyncPurchaseProductsCallback;

	void Update(uint64_t ms);

	void AddListener(RailPurchaseStoreEvent* listener);
	void RemoveListener(RailPurchaseStoreEvent* listener);

	bool AsyncRequestAllPurchasableProducts();
	bool GetProductInfo(rail::RailProductID product_id,
		rail::RailPurchaseProductInfo* product);
	bool AsyncPurchaseProductsToAssets(const rail::RailArray<rail::RailProductItem>& cart_items,
		const std::string& user_data = "");
	bool AsyncPurchaseProducts(const rail::RailArray<rail::RailProductItem>& cart_items,
		const std::string& user_data = "");
	bool AsyncFinishOrder(const std::string& order_id);

	virtual void OnRailEvent(rail::RAIL_EVENT_ID event_id, rail::EventBase* param);

private:
	bool Inital();
	void RunRequestAllPurchasableProductsCallback(bool is_succ, Local<Array> productsInfo);
	void RunAsyncPurchaseProductsCallback(bool is_succ, std::string orderId);
	void OnRailInGamePurchaseRequestAllPurchasableProductsResponse(
		rail::rail_event::RailInGamePurchaseRequestAllPurchasableProductsResponse* res);
	void OnRailInGamePurchasePurchaseProductsToAssetsResponse(
		rail::rail_event::RailInGamePurchasePurchaseProductsToAssetsResponse* res);
	void OnRailInGamePurchasePurchaseProductsResponse(
		rail::rail_event::RailInGamePurchasePurchaseProductsResponse* res);
	void OnRailInGamePurchaseFinishOrderResponse(
		rail::rail_event::RailInGamePurchaseFinishOrderResponse* res);
private:
	ProductInfoMap product_info_map_;
	std::set<RailPurchaseStoreEvent*> listeners_;
};

#endif  // SDK_WRAPPER_RAIL_PURCHASE_STORE_H
