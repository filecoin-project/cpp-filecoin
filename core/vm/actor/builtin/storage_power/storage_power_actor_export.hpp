/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
#define CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/miner/types.hpp"
#include "vm/actor/builtin/storage_power/policy.hpp"
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::actor::builtin::storage_power {

  using miner::PeerId;
  using runtime::InvocationOutput;
  using runtime::Runtime;

  constexpr MethodNumber kAddBalanceMethodNumber{2};
  constexpr MethodNumber kWithdrawBalanceMethodNumber{3};
  constexpr MethodNumber kCreateMinerMethodNumber{4};
  constexpr MethodNumber kDeleteMinerMethodNumber{5};
  constexpr MethodNumber kOnSectorProveCommitMethodNumber{6};
  constexpr MethodNumber kOnSectorTerminateMethodNumber{7};
  constexpr MethodNumber kOnSectorTemporaryFaultEffectiveBeginMethodNumber{8};
  constexpr MethodNumber kOnSectorTemporaryFaultEffectiveEndMethodNumber{9};
  constexpr MethodNumber kOnSectorModifyWeightDescMethodNumber{10};
  constexpr MethodNumber kOnMinerSurprisePoStSuccessMethodNumber{11};
  constexpr MethodNumber kOnMinerSurprisePoStFailureMethodNumber{12};
  constexpr MethodNumber kEnrollCronEventMethodNumber{13};
  constexpr MethodNumber kReportConsensusFaultMethodNumber{14};
  constexpr MethodNumber kOnEpochTickEndMethodNumber{15};

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

  struct OnSectorProveCommitReturn {
    TokenAmount pledge;
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

  struct OnSectorModifyWeightDescParams {
    SectorStorageWeightDesc prev_weight;
    TokenAmount prev_pledge;
    SectorStorageWeightDesc new_weight;
  };

  struct OnMinerWindowedPoStFailureParams {
    uint64_t num_consecutive_failures;
  };

  struct EnrollCronEventParams {
    ChainEpoch event_epoch;
    Buffer payload;
  };

  class StoragePowerActorMethods {
   public:
    static ACTOR_METHOD(construct);

    static ACTOR_METHOD(addBalance);

    static ACTOR_METHOD(withdrawBalance);

    static ACTOR_METHOD(createMiner);

    static ACTOR_METHOD(deleteMiner);

    static ACTOR_METHOD(onSectorProveCommit);

    static ACTOR_METHOD(onSectorTerminate);

    static ACTOR_METHOD(onSectorTemporaryFaultEffectiveBegin);

   private:
    /**
     * Get current storage power actor state
     * @param runtime - current runtime
     * @return current storage power actor state or appropriate error
     */
    static outcome::result<StoragePowerActor> getCurrentState(Runtime &runtime);

    static outcome::result<InvocationOutput> slashPledgeCollateral(
        Runtime &runtime,
        StoragePowerActor &power_actor,
        Address miner,
        TokenAmount to_slash);

    /**
     * Deletes miner from state and slashes miner balance
     * @param runtime - current runtime
     * @param state - current storage power actor state
     * @param miner address to delete
     * @return error in case of failure
     */
    static outcome::result<void> deleteMinerActor(Runtime &runtime,
                                                  StoragePowerActor &state,
                                                  const Address &miner);
  };

  /** Exported StoragePowerActor methods to invoker */
  extern const ActorExports exports;

  CBOR_TUPLE(AddBalanceParameters, miner)

  CBOR_TUPLE(WithdrawBalanceParameters, miner, requested)

  CBOR_TUPLE(CreateMinerParameters, worker, sector_size, peer_id)
  CBOR_TUPLE(CreateMinerReturn, id_address, robust_address)

  CBOR_TUPLE(DeleteMinerParameters, miner)

  CBOR_TUPLE(OnSectorProveCommitParameters, weight)
  CBOR_TUPLE(OnSectorProveCommitReturn, pledge)

  CBOR_TUPLE(OnSectorTerminateParameters, termination_type, weights, pledge)

  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveBeginParameters, weights, pledge)

  CBOR_TUPLE(OnSectorTemporaryFaultEffectiveEndParameters, weights, pledge)

  CBOR_TUPLE(OnSectorModifyWeightDescParams,
             prev_weight,
             prev_pledge,
             new_weight)

  CBOR_TUPLE(OnMinerWindowedPoStFailureParams, num_consecutive_failures)

  CBOR_TUPLE(EnrollCronEventParams, event_epoch, payload);

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
