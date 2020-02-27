/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/miner/types.hpp"

namespace fc::vm::actor::builtin::miner {

  constexpr MethodNumber kSubmitElectionPoStMethodNumber{20};

  struct Construct : ActorMethodBase<1> {
    using Params = ConstructorParams;
    ACTOR_METHOD_STUB();
  };

  struct ControlAddresses : ActorMethodBase<2> {
    using Result = GetControlAddressesReturn;
    ACTOR_METHOD_STUB();
  };

  struct ChangeWorkerAddress : ActorMethodBase<3> {
    using Params = ChangeWorkerAddressParams;
    ACTOR_METHOD_STUB();
  };

  struct ChangePeerId : ActorMethodBase<4> {
    using Params = ChangePeerIdParams;
    ACTOR_METHOD_STUB();
  };

  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    using Params = SubmitWindowedPoStParams;
    ACTOR_METHOD_STUB();
  };

  struct OnDeleteMiner : ActorMethodBase<6> {
    ACTOR_METHOD_STUB();
  };

  struct PreCommitSector : ActorMethodBase<7> {
    using Params = PreCommitSectorParams;
    ACTOR_METHOD_STUB();
  };

  struct ProveCommitSector : ActorMethodBase<8> {
    using Params = ProveCommitSectorParams;
    ACTOR_METHOD_STUB();
  };

  struct ExtendSectorExpiration : ActorMethodBase<9> {
    using Params = ExtendSectorExpirationParams;
    ACTOR_METHOD_STUB();
  };

  struct TerminateSectors : ActorMethodBase<10> {
    using Params = TerminateSectorsParams;
    ACTOR_METHOD_STUB();
  };

  struct DeclareTemporaryFaults : ActorMethodBase<11> {
    using Params = DeclareTemporaryFaultsParams;
    ACTOR_METHOD_STUB();
  };

  struct OnDeferredCronEvent : ActorMethodBase<12> {
    using Params = OnDeferredCronEventParams;
    ACTOR_METHOD_STUB();
  };

  struct CheckSectorProven : ActorMethodBase<13> {
    using Params = CheckSectorProvenParams;
    ACTOR_METHOD_STUB();
  };

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
