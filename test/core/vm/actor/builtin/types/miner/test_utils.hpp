/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"

namespace fc::vm::actor::builtin {
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using types::miner::ExpirationQueue;
  using types::miner::SectorOnChainInfo;

  class Utils {
   public:
    static SectorOnChainInfo testSector(ChainEpoch expiration,
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

    static void requireNoExpirationGroupsBefore(ChainEpoch epoch,
                                                ExpirationQueue &queue) {
      EXPECT_OUTCOME_TRUE(es, queue.popUntil(epoch - 1));
      EXPECT_TRUE(es.isEmpty());
    }

    template <typename T>
    static std::vector<T> slice(const std::vector<T> &source,
                                int begin,
                                int end = -1) {
      typename std::vector<T>::const_iterator begin_it = source.begin() + begin;
      typename std::vector<T>::const_iterator end_it =
          end == -1 ? source.end() : source.begin() + end;
      return std::vector<T>(begin_it, end_it);
    }
  };

}  // namespace fc::vm::actor::builtin