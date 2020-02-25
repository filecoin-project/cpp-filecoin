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

  constexpr MethodNumber kGetControlAddressesMethodNumber{2};
  constexpr MethodNumber kChangeWorkerAddressMethodNumber{3};
  constexpr MethodNumber kChangePeerIdMethodNumber{4};
  constexpr MethodNumber kSubmitWindowedPoStMethodNumber{5};
  constexpr MethodNumber kOnDeleteMinerMethodNumber{6};
  constexpr MethodNumber kPreCommitSectorMethodNumber{7};
  constexpr MethodNumber kProveCommitSectorMethodNumber{8};
  constexpr MethodNumber kExtendSectorExpirationMethodNumber{9};
  constexpr MethodNumber kTerminateSectorsMethodNumber{10};
  constexpr MethodNumber kDeclareTemporaryFaultsMethodNumber{11};
  constexpr MethodNumber kOnDeferredCronEventMethodNumber{11};

  constexpr MethodNumber kSubmitElectionPoStMethodNumber{20};

  ACTOR_METHOD(constructor);

  ACTOR_METHOD(controlAdresses);

  ACTOR_METHOD(changeWorkerAddress);

  ACTOR_METHOD(changePeerId);

  ACTOR_METHOD(submitWindowedPoSt);

  ACTOR_METHOD(onDeleteMiner);

  ACTOR_METHOD(preCommitSector);

  ACTOR_METHOD(proveCommitSector);

  ACTOR_METHOD(extendSectorExpiration);

  ACTOR_METHOD(terminateSectors);

  ACTOR_METHOD(declareTemporaryFaults);

  ACTOR_METHOD(onDeferredCronEvent);

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
