/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include "primitives/types.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin {
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using types::miner::ExpirationQueue;
  using types::miner::SectorOnChainInfo;

  inline SectorOnChainInfo testSector(ChainEpoch expiration,
                                      SectorNumber number,
                                      DealWeight &&weight,
                                      DealWeight &&vweight,
                                      TokenAmount &&pledge) {
    SectorOnChainInfo sector;

    sector.expiration = expiration;
    sector.sector = number;
    sector.sealed_cid = kEmptyObjectCid;
    sector.deal_weight = std::move(weight);
    sector.verified_deal_weight = std::move(vweight);
    sector.init_pledge = std::move(pledge);

    return sector;
  }

  inline void requireNoExpirationGroupsBefore(ChainEpoch epoch,
                                              ExpirationQueue &queue) {
    EXPECT_OUTCOME_TRUE(es, queue.popUntil(epoch - 1));
    EXPECT_TRUE(es.isEmpty());
  }

  inline std::vector<SectorOnChainInfo> selectSectorsTest(
      const std::vector<SectorOnChainInfo> &sectors, const RleBitset &field) {
    auto to_include = field;
    std::vector<SectorOnChainInfo> included;

    for (const auto &sector : sectors) {
      if (!to_include.has(sector.sector)) {
        continue;
      }
      included.push_back(sector);
      to_include.erase(sector.sector);
    }
    EXPECT_TRUE(to_include.empty());
    return included;
  }

}  // namespace fc::vm::actor::builtin
