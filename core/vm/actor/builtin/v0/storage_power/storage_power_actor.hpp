/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/types/storage_power/policy.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using common::smoothing::FilterEstimate;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::SectorStorageWeightDesc;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;

  constexpr auto kErrTooManyProveCommits =
      VMExitCode::kErrFirstActorSpecificExitCode;

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct CreateMiner : ActorMethodBase<2> {
    struct Params {
      Address owner;
      Address worker;
      RegisteredSealProof seal_proof_type;
      Buffer peer_id;
      std::vector<Multiaddress> multiaddresses;
    };

    struct Result {
      Address id_address;      // The canonical ID-based address for the actor
      Address robust_address;  // A more expensive but re-org-safe address for
                               // the newly created actor
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CreateMiner::Params,
             owner,
             worker,
             seal_proof_type,
             peer_id,
             multiaddresses)
  CBOR_TUPLE(CreateMiner::Result, id_address, robust_address)

  struct UpdateClaimedPower : ActorMethodBase<3> {
    struct Params {
      StoragePower raw_byte_delta;
      StoragePower quality_adjusted_delta;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(UpdateClaimedPower::Params, raw_byte_delta, quality_adjusted_delta)

  struct EnrollCronEvent : ActorMethodBase<4> {
    struct Params {
      ChainEpoch event_epoch;
      Buffer payload;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(EnrollCronEvent::Params, event_epoch, payload)

  struct OnEpochTickEnd : ActorMethodBase<5> {
    ACTOR_METHOD_DECL();
  };

  struct UpdatePledgeTotal : ActorMethodBase<6> {
    using Params = TokenAmount;  // pledgeDelta
    ACTOR_METHOD_DECL();
  };

  struct OnConsensusFault : ActorMethodBase<7> {
    using Params = TokenAmount;  // pledgeAmount
    ACTOR_METHOD_DECL();
  };

  struct SubmitPoRepForBulkVerify : ActorMethodBase<8> {
    using Params = SealVerifyInfo;
    ACTOR_METHOD_DECL();
  };

  struct CurrentTotalPower : ActorMethodBase<9> {
    struct Result {
      StoragePower raw_byte_power;
      StoragePower quality_adj_power;
      TokenAmount pledge_collateral;
      FilterEstimate quality_adj_power_smoothed;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CurrentTotalPower::Result,
             raw_byte_power,
             quality_adj_power,
             pledge_collateral,
             quality_adj_power_smoothed)

  inline bool operator==(const CreateMiner::Result &lhs,
                         const CreateMiner::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::storage_power
