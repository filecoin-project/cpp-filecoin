/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/block/block.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/storage_power/policy.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using miner::PeerId;
  using primitives::TokenAmount;
  using primitives::block::BlockHeader;

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct AddBalance : ActorMethodBase<2> {
    struct Params {
      Address miner;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(AddBalance::Params, miner)

  struct WithdrawBalance : ActorMethodBase<3> {
    struct Params {
      Address miner;
      TokenAmount requested;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(WithdrawBalance::Params, miner, requested)

  struct CreateMiner : ActorMethodBase<4> {
    struct Params {
      Address worker;  // must be an ID-address
      uint64_t sector_size;
      PeerId peer_id;
    };

    struct Result {
      Address id_address;      // The canonical ID-based address for the actor
      Address robust_address;  // A mre expensive but re-org-safe address for
                               // the newly created actor
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CreateMiner::Params, worker, sector_size, peer_id)
  CBOR_TUPLE(CreateMiner::Result, id_address, robust_address)

  struct DeleteMiner : ActorMethodBase<5> {
    struct Params {
      Address miner;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeleteMiner::Params, miner)

  struct OnSectorProveCommit : ActorMethodBase<6> {
    struct Params {
      SectorStorageWeightDesc weight;
    };
    using Result = TokenAmount;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorProveCommit::Params, weight)

  struct OnSectorTerminate : ActorMethodBase<7> {
    struct Params {
      SectorTerminationType termination_type;
      std::vector<SectorStorageWeightDesc> weights;
      TokenAmount pledge;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorTerminate::Params, termination_type, weights, pledge)

  struct OnSectorTemporaryFaultEffectiveBegin : ActorMethodBase<8> {
    struct Params {
      std::vector<SectorStorageWeightDesc> weights;
      TokenAmount pledge;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveBegin::Params, weights, pledge)

  struct OnSectorTemporaryFaultEffectiveEnd : ActorMethodBase<9> {
    struct Params {
      std::vector<SectorStorageWeightDesc> weights;
      TokenAmount pledge;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveEnd::Params, weights, pledge)

  struct OnSectorModifyWeightDesc : ActorMethodBase<10> {
    struct Params {
      SectorStorageWeightDesc prev_weight;
      TokenAmount prev_pledge;
      SectorStorageWeightDesc new_weight;
    };
    using Result = TokenAmount;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnSectorModifyWeightDesc::Params,
             prev_weight,
             prev_pledge,
             new_weight)

  struct OnMinerWindowedPoStSuccess : ActorMethodBase<11> {
    ACTOR_METHOD_DECL();
  };

  struct OnMinerWindowedPoStFailure : ActorMethodBase<12> {
    struct Params {
      uint64_t num_consecutive_failures;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnMinerWindowedPoStFailure::Params, num_consecutive_failures)

  struct EnrollCronEvent : ActorMethodBase<13> {
    struct Params {
      ChainEpoch event_epoch;
      Buffer payload;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(EnrollCronEvent::Params, event_epoch, payload)

  struct ReportConsensusFault : ActorMethodBase<14> {
    struct Params {
      BlockHeader block_header_1;
      BlockHeader block_header_2;
      Address target;
      ChainEpoch fault_epoch;
      ConsensusFaultType fault_type;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ReportConsensusFault::Params,
             block_header_1,
             block_header_2,
             target,
             fault_epoch,
             fault_type)

  struct OnEpochTickEnd : ActorMethodBase<15> {
    ACTOR_METHOD_DECL();
  };

  inline bool operator==(const CreateMiner::Result &lhs,
                         const CreateMiner::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

  /** Exported StoragePowerActor methods to invoker */
  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
