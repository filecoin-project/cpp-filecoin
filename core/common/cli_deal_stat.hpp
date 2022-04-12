/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include "api/rpc/json.hpp"

namespace fc {
  using api::StorageMarketDealInfo;
  using codec::json::Set;
  using codec::json::Value;
  using markets::storage::StorageDeal;

  struct CliDealStat {
    StorageMarketDealInfo deal_info;
    StorageDeal deal;
  };
  JSON_ENCODE(CliDealStat) {
    Value j{rapidjson::kObjectType};
    Set(j, "DealInfo", v.deal_info, allocator);
    Set(j, "OnChain", v.deal, allocator);
    return j;
  }
}  // namespace fc