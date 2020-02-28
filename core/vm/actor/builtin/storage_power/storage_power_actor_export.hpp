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

  struct AddBalanceParameters {
    Address miner;
  };

  struct WithdrawBalanceParameters {
    Address miner;
    TokenAmount requested;
  };

  struct CreateMinerParameters {
    Address worker;  // must be an ID-address
    uint64_t sector_size;
    PeerId peer_id;
  };

  struct CreateMinerReturn {
    Address id_address;      // The canonical ID-based address for the actor
    Address robust_address;  // A mre expensive but re-org-safe address for the
                             // newly created actor
  };

  struct DeleteMinerParameters {
    Address miner;
  };

  struct OnSectorProveCommitParameters {
    SectorStorageWeightDesc weight;
  };

  struct OnSectorTerminateParameters {
    SectorTerminationType termination_type;
    std::vector<SectorStorageWeightDesc> weights;
    TokenAmount pledge;
  };

  struct OnSectorTemporaryFaultEffectiveBeginParameters {
    std::vector<SectorStorageWeightDesc> weights;
    TokenAmount pledge;
  };

  struct OnSectorTemporaryFaultEffectiveEndParameters {
    std::vector<SectorStorageWeightDesc> weights;
    TokenAmount pledge;
  };

  struct OnSectorModifyWeightDescParameters {
    SectorStorageWeightDesc prev_weight;
    TokenAmount prev_pledge;
    SectorStorageWeightDesc new_weight;
  };

  struct OnSectorModifyWeightDescReturn {
    TokenAmount new_pledge;
  };

  struct OnMinerWindowedPoStFailureParameters {
    uint64_t num_consecutive_failures;
  };

  struct EnrollCronEventParameters {
    ChainEpoch event_epoch;
    Buffer payload;
  };

  struct ReportConsensusFaultParameters {
    BlockHeader block_header_1;
    BlockHeader block_header_2;
    Address target;
    ChainEpoch fault_epoch;
    ConsensusFaultType fault_type;
  };

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_STUB();
  };

  struct AddBalance : ActorMethodBase<2> {
    using Params = AddBalanceParameters;
    ACTOR_METHOD_STUB();
  };

  struct WithdrawBalance : ActorMethodBase<3> {
    using Params = WithdrawBalanceParameters;
    ACTOR_METHOD_STUB();
  };

  struct CreateMiner : ActorMethodBase<4> {
    using Params = CreateMinerParameters;
    using Result = CreateMinerReturn;
    ACTOR_METHOD_STUB();
  };

  struct DeleteMiner : ActorMethodBase<5> {
    using Params = DeleteMinerParameters;
    ACTOR_METHOD_STUB();
  };

  struct OnSectorProveCommit : ActorMethodBase<6> {
    using Params = OnSectorProveCommitParameters;
    using Result = TokenAmount;
    ACTOR_METHOD_STUB();
  };

  struct OnSectorTerminate : ActorMethodBase<7> {
    using Params = OnSectorTerminateParameters;
    ACTOR_METHOD_STUB();
  };

  struct OnSectorTemporaryFaultEffectiveBegin : ActorMethodBase<8> {
    using Params = OnSectorTemporaryFaultEffectiveBeginParameters;
    ACTOR_METHOD_STUB();
  };

  struct OnSectorTemporaryFaultEffectiveEnd : ActorMethodBase<9> {
    using Params = OnSectorTemporaryFaultEffectiveEndParameters;
    ACTOR_METHOD_STUB();
  };

  struct OnSectorModifyWeightDesc : ActorMethodBase<10> {
    using Params = OnSectorModifyWeightDescParameters;
    using Result = TokenAmount;
    ACTOR_METHOD_STUB();
  };

  struct OnMinerWindowedPoStSuccess : ActorMethodBase<11> {
    ACTOR_METHOD_STUB();
  };

  struct OnMinerWindowedPoStFailure : ActorMethodBase<12> {
    using Params = OnMinerWindowedPoStFailureParameters;
    ACTOR_METHOD_STUB();
  };

  struct EnrollCronEvent : ActorMethodBase<13> {
    using Params = EnrollCronEventParameters;
    ACTOR_METHOD_STUB();
  };

  struct ReportConsensusFault : ActorMethodBase<14> {
    using Params = ReportConsensusFaultParameters;
    ACTOR_METHOD_STUB();
  };

  struct OnEpochTickEnd : ActorMethodBase<15> {
    ACTOR_METHOD_STUB();
  };

  /** Exported StoragePowerActor methods to invoker */
  extern const ActorExports exports;

  CBOR_TUPLE(AddBalanceParameters, miner)

  CBOR_TUPLE(WithdrawBalanceParameters, miner, requested)

  CBOR_TUPLE(CreateMinerParameters, worker, sector_size, peer_id)
  CBOR_TUPLE(CreateMinerReturn, id_address, robust_address)

  CBOR_TUPLE(DeleteMinerParameters, miner)

  CBOR_TUPLE(OnSectorProveCommitParameters, weight)

  CBOR_TUPLE(OnSectorTerminateParameters, termination_type, weights, pledge)

  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveBeginParameters, weights, pledge)

  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveEndParameters, weights, pledge)

  CBOR_TUPLE(OnSectorModifyWeightDescParameters,
             prev_weight,
             prev_pledge,
             new_weight)
  CBOR_TUPLE(OnSectorModifyWeightDescReturn, new_pledge)

  CBOR_TUPLE(OnMinerWindowedPoStFailureParameters, num_consecutive_failures)

  CBOR_TUPLE(EnrollCronEventParameters, event_epoch, payload)

  CBOR_TUPLE(ReportConsensusFaultParameters,
             block_header_1,
             block_header_2,
             target,
             fault_epoch,
             fault_type)

  inline bool operator==(const CreateMiner::Result &lhs,
                         const CreateMiner::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
