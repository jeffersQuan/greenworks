// Copyright (c) 2019, Entropy Game Global Limited.
// All rights reserved.

#ifndef RAIL_SDK_RAIL_IN_GAME_COIN_H
#define RAIL_SDK_RAIL_IN_GAME_COIN_H

#include "rail/sdk/rail_in_game_coin_define.h"

namespace rail {
#pragma pack(push, RAIL_SDK_PACKING)

class IRailInGameCoin {
  public:
    // callback is RailInGameCoinRequestCoinInfoResponse
    virtual RailResult AsyncRequestCoinInfo(const RailString& user_data) = 0;

    // callback is RailInGameCoinPurchaseCoinsResponse
    virtual RailResult AsyncPurchaseCoins(const RailCoins& purchase_info,
                        const RailString& user_data) = 0;
};

#pragma pack(pop)
}  // namespace rail

#endif  // RAIL_SDK_RAIL_IN_GAME_COIN_H
