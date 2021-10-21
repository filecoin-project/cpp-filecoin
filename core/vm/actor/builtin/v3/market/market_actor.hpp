/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v2/market/market_actor.hpp"

#include "vm/actor/builtin/types/market/sector_deals.hpp"
#include "vm/actor/builtin/types/market/sector_weights.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using types::market::SectorDeals;
  using types::market::SectorWeights;

  using Construct = v2::market::Construct;
  using AddBalance = v2::market::AddBalance;
  using WithdrawBalance = v2::market::WithdrawBalance;
  using PublishStorageDeals = v2::market::PublishStorageDeals;

  struct VerifyDealsForActivation : ActorMethodBase<5> {
    struct Params {
      std::vector<SectorDeals> sectors;
    };
    struct Result {
      std::vector<SectorWeights> sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsForActivation::Params, sectors)
  CBOR_TUPLE(VerifyDealsForActivation::Result, sectors)

  using ActivateDeals = v2::market::ActivateDeals;
  using OnMinerSectorsTerminate = v2::market::OnMinerSectorsTerminate;
  using ComputeDataCommitment = v2::market::ComputeDataCommitment;
  using CronTick = v2::market::CronTick;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v3::market
