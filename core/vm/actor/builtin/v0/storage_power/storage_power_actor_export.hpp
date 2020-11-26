/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP

#include "vm/actor/builtin/v0/storage_power/policy.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::v0::storage_power {
  using primitives::ChainEpoch;
  using primitives::SectorStorageWeightDesc;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredProof;

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct CreateMiner : ActorMethodBase<2> {
    struct Params {
      Address owner, worker;
      RegisteredProof seal_proof_type;
      Buffer peer_id;
    };

    struct Result {
      Address id_address;      // The canonical ID-based address for the actor
      Address robust_address;  // A mre expensive but re-org-safe address for
                               // the newly created actor
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CreateMiner::Params, owner, worker, seal_proof_type, peer_id)
  CBOR_TUPLE(CreateMiner::Result, id_address, robust_address)

  struct DeleteMiner : ActorMethodBase<3> {
    struct Params {
      Address miner;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeleteMiner::Params, miner)

  struct OnSectorProveCommit : ActorMethodBase<4> {
    struct Params {
      SectorStorageWeightDesc weight;
    };
    using Result = TokenAmount;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorProveCommit::Params, weight)

  struct OnSectorTerminate : ActorMethodBase<5> {
    struct Params {
      SectorTerminationType termination_type;
      std::vector<SectorStorageWeightDesc> weights;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorTerminate::Params, termination_type, weights)

  struct OnFaultBegin : ActorMethodBase<6> {
    struct Params {
      std::vector<SectorStorageWeightDesc> weights;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnFaultBegin::Params, weights)

  struct OnFaultEnd : ActorMethodBase<7> {
    struct Params {
      std::vector<SectorStorageWeightDesc> weights;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnFaultEnd::Params, weights)

  struct OnSectorModifyWeightDesc : ActorMethodBase<8> {
    struct Params {
      SectorStorageWeightDesc prev_weight;
      SectorStorageWeightDesc new_weight;
    };
    using Result = TokenAmount;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorModifyWeightDesc::Params, prev_weight, new_weight)

  struct CurrentTotalPower : ActorMethodBase<9> {
    /**
     * Alpha Beta Filter "position" (value) and "velocity" (rate of change of
     * value) estimates
     * Estimates are in Q.128 format
     */
    struct FilterEstimate {
      BigInt position_estimate;
      BigInt velocity_estimate;
    };
    struct Result {
      StoragePower raw_byte_power;
      StoragePower quality_adj_power;
      TokenAmount pledge_collateral;
      FilterEstimate quality_adj_power_smoothed;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CurrentTotalPower::FilterEstimate,
             position_estimate,
             velocity_estimate)
  CBOR_TUPLE(CurrentTotalPower::Result,
             raw_byte_power,
             quality_adj_power,
             pledge_collateral,
             quality_adj_power_smoothed)

  // TODO (a.chernyshov) update storage power actor
  struct EnrollCronEvent : ActorMethodBase<13> {
    struct Params {
      ChainEpoch event_epoch;
      Buffer payload;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(EnrollCronEvent::Params, event_epoch, payload)

  struct OnEpochTickEnd : ActorMethodBase<10> {
    ACTOR_METHOD_DECL();
  };

  struct UpdatePledgeTotal : ActorMethodBase<11> {
    using Params = TokenAmount;
    ACTOR_METHOD_DECL();
  };

  struct OnConsensusFault : ActorMethodBase<12> {
    using Params = TokenAmount;
    ACTOR_METHOD_DECL();
  };

  inline bool operator==(const CreateMiner::Result &lhs,
                         const CreateMiner::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::v0::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
