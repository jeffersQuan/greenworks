// Copyright (c) 2019, Entropy Game Global Limited.
// All rights reserved.

#ifndef RAIL_SDK_RAIL_IN_GAME_COIN_DEFINE_H
#define RAIL_SDK_RAIL_IN_GAME_COIN_DEFINE_H

#include "rail/sdk/base/rail_define.h"
#include "rail/sdk/rail_event.h"

namespace rail {
#pragma pack(push, RAIL_SDK_PACKING)

struct RailCurrencyExchangeCoinRate {
    RailCurrencyExchangeCoinRate() {
        to_exchange_coins = 0;
        pay_price = 0.0;
    }

    RailString currency;
    uint32_t to_exchange_coins;
    float pay_price;
};

struct RailCoinInfo {
    RailCoinInfo() {
    }

    RailString name;
    RailString description;
    RailString icon_url;
    RailCurrencyExchangeCoinRate exchange_rate;
};

struct RailCoins {
    RailCoins() {
        total_price = 0.0;
        quantity = 0;
    };

    float total_price;  // the total price that player need to pay.
    uint32_t quantity;  // the quantity of coins that player can get.
};

namespace rail_event {

struct RailInGameCoinRequestCoinInfoResponse :
    RailEvent<kRailEventInGameCoinRequestCoinInfoResult> {
    RailInGameCoinRequestCoinInfoResponse() {
        result = kFailure;
    }

    RailCoinInfo coin_info;
};

struct RailInGameCoinPurchaseCoinsResponse :
    RailEvent<kRailEventInGameCoinPurchaseCoinsResult> {
    RailInGameCoinPurchaseCoinsResponse() {
        result = kFailure;
    }
};

}  // namespace rail_event

#pragma pack(pop)
}  // namespace rail

#endif  // RAIL_SDK_RAIL_IN_GAME_COIN_DEFINE_H
