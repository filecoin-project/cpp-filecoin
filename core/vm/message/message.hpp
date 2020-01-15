/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_HPP

#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::message {

  using primitives::BigInt;
  using primitives::address::Address;

  struct UnsignedMessage {
    Address to;
    Address from;

    uint64_t nonce{};

    BigInt value{};

    uint64_t method{};
  };

}

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_HPP
