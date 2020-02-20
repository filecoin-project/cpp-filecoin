/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MINER_ACTOR_HPP
#define CPP_FILECOIN_MINER_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::miner {

  constexpr MethodNumber kGetControlAddresses{2};

  struct GetControlAddressesReturn {
    Address owner;
    Address worker;
  };

  CBOR_ENCODE(GetControlAddressesReturn, ret) {
    return s << (s.list() << ret.owner << ret.worker);
  }

  CBOR_DECODE(GetControlAddressesReturn, ret) {
    s.list() >> ret.owner >> ret.worker;
    return s;
  }

}  // namespace fc::vm::actor::builtin::miner

#endif  // CPP_FILECOIN_MINER_ACTOR_HPP
