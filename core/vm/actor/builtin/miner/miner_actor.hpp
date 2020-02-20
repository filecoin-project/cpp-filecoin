/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/miner/types.hpp"

namespace fc::vm::actor::builtin::miner {

  constexpr MethodNumber kGetControlAddresses{2};
  constexpr MethodNumber kSubmitElectionPoStMethodNumber{20};

  struct GetControlAddressesReturn {
    Address owner;
    Address worker;
  };

  CBOR_TUPLE(GetControlAddressesReturn, owner, worker);

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MINER_MINER_ACTOR_HPP
