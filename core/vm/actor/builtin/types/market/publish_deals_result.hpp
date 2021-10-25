/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/actor/builtin/v6/market/market_actor.hpp"

namespace fc::vm::actor::builtin::types::market {
  inline outcome::result<DealId> publishDealsResult(BytesIn cbor,
                                                    ActorVersion version,
                                                    size_t index) {
    if (version < ActorVersion::kVersion6) {
      OUTCOME_TRY(
          res,
          codec::cbor::decode<v0::market::PublishStorageDeals::Result>(cbor));
      if (index < res.deals.size()) {
        return res.deals[index];
      }
    } else {
      OUTCOME_TRY(
          res,
          codec::cbor::decode<v6::market::PublishStorageDeals::Result>(cbor));
      const auto it{res.valid.find(index)};
      if (it != res.valid.end()) {
        const auto i{gsl::narrow<size_t>(std::distance(res.valid.begin(), it))};
        if (i < res.deals.size()) {
          return res.deals[i];
        }
      }
    }
    return ERROR_TEXT("publishDealsResult invalid deal");
  }
}  // namespace fc::vm::actor::builtin::types::market
