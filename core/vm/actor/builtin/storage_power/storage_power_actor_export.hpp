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
#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/runtime/runtime.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::actor::builtin::storage_power {

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

  /**
   * Construct method parameters
   * Storage miner actor constructor params are defined here so the power actor
   * can send them to the init actor to instantiate miners
   */
  struct ConstructParameters {
    Address owner;
    Address worker;
    uint64_t sector_size;
    std::string peer_id;
  };

  struct AddBalanceParameters {
    Address miner;
  };

  struct WithdrawBalanceParameters {
    Address miner;
    TokenAmount requested;
  };

  class StoragePowerActorMethods {
   public:
    static outcome::result<InvocationOutput> construct(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> addBalance(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> withdrawBalance(
        const Actor &actor, Runtime &runtime, const MethodParams &params);
  };

  /** Exported StoragePowerActor methods to invoker */
  extern const ActorExports exports;

  CBOR_TUPLE(ConstructParameters, owner, worker, sector_size, peer_id);

  CBOR_TUPLE(AddBalanceParameters, miner);

  CBOR_TUPLE(WithdrawBalanceParameters, miner, requested);

}  // namespace fc::vm::actor::builtin::storage_power

#endif  // CPP_FILECOIN_VM_ACTOR_BUILTIN_STORAGE_POWER_ACTOR_HPP
