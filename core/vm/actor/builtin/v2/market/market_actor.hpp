/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/market/market_actor.hpp"

namespace fc::vm::actor::builtin::v2::market {
  using primitives::DealWeight;

  using Construct = v0::market::Construct;
  using AddBalance = v0::market::AddBalance;
  using WithdrawBalance = v0::market::WithdrawBalance;
  using PublishStorageDeals = v0::market::PublishStorageDeals;

  struct VerifyDealsForActivation : ActorMethodBase<5> {
    using Params = v0::market::VerifyDealsForActivation::Params;
    struct Result {
      DealWeight deal_weight;
      DealWeight verified_deal_weight;
      uint64_t deal_space;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsForActivation::Result,
             deal_weight,
             verified_deal_weight,
             deal_space)

  using ActivateDeals = v0::market::ActivateDeals;
  using OnMinerSectorsTerminate = v0::market::OnMinerSectorsTerminate;
  using ComputeDataCommitment = v0::market::ComputeDataCommitment;
  using CronTick = v0::market::CronTick;

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v2::market
