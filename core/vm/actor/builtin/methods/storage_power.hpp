/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/actor/builtin/types/storage_power/miner_params.hpp"

namespace fc::vm::actor::builtin::power {
  using common::smoothing::FilterEstimate;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredPoStProof;
  using primitives::sector::SealVerifyInfo;
  using types::storage_power::CreateMinerParams;

  // These methods must be actual with the last version of actors

  enum class PowerActor : MethodNumber {
    kConstruct = 1,
    kCreateMiner,
    kUpdateClaimedPower,
    kEnrollCronEvent,
    kCronTick,  // since v7, OnEpochTickEnd for v6 and early
    kUpdatePledgeTotal,
    kOnConsensusFault,  // deprecated since v2
    kSubmitPoRepForBulkVerify,
    kCurrentTotalPower,
  }

  struct Construct : ActorMethodBase<PowerActor::kConstruct> {};

  struct CreateMiner : ActorMethodBase<PowerActor::kCreateMiner> {
    using Params = CreateMinerParams;

    struct Result {
      Address id_address;
      Address robust_address;

      inline bool operator==(const Result &other) const {
        return id_address == other.id_address
               && robust_address == other.robust_address;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(CreateMiner::Result, id_address, robust_address)

  struct UpdateClaimedPower : ActorMethodBase<PowerActor::kUpdateClaimedPower> {
    struct Params {
      StoragePower raw_byte_delta;
      StoragePower quality_adjusted_delta;

      inline bool operator==(const Params &other) const {
        return raw_byte_delta == other.raw_byte_delta
               && quality_adjusted_delta == other.quality_adjusted_delta;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(UpdateClaimedPower::Params, raw_byte_delta, quality_adjusted_delta)

  struct EnrollCronEvent : ActorMethodBase<PowerActor::kEnrollCronEvent> {
    struct Params {
      ChainEpoch event_epoch{};
      Bytes payload;

      inline bool operator==(const Params &other) const {
        return event_epoch == other.event_epoch && payload == other.payload;
      }

      inline bool operator!=(const Params &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(EnrollCronEvent::Params, event_epoch, payload)

  struct CronTick : ActorMethodBase<PowerActor::kCronTick> {};

  struct UpdatePledgeTotal : ActorMethodBase<PowerActor::kUpdatePledgeTotal> {
    using Params = TokenAmount;
  };

  struct OnConsensusFault : ActorMethodBase<PowerActor::kOnConsensusFault> {
    using Params = TokenAmount;
  };

  struct SubmitPoRepForBulkVerify
      : ActorMethodBase<PowerActor::kSubmitPoRepForBulkVerify> {
    using Params = SealVerifyInfo;
  };

  struct CurrentTotalPower : ActorMethodBase<PowerActor::kCurrentTotalPower> {
    struct Result {
      StoragePower raw_byte_power;
      StoragePower quality_adj_power;
      TokenAmount pledge_collateral;
      FilterEstimate quality_adj_power_smoothed;

      inline bool operator==(const Result &other) const {
        return raw_byte_power == other.raw_byte_power
               && quality_adj_power == other.quality_adj_power
               && pledge_collateral == other.pledge_collateral
               && quality_adj_power_smoothed
                      == other.quality_adj_power_smoothed;
      }

      inline bool operator!=(const Result &other) const {
        return !(*this == other);
      }
    };
  };
  CBOR_TUPLE(CurrentTotalPower::Result,
             raw_byte_power,
             quality_adj_power,
             pledge_collateral,
             quality_adj_power_smoothed)

}  // namespace fc::vm::actor::builtin::power
