/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_HPP

#include "common/buffer.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

// TODO (yuraz): FIL-72 not really used yet, just a sketch remove this message when
// real structures  implemented

namespace fc::primitives::chain {
  struct Message {
    address::Address to;
    address::Address from;
    uint64_t nonce;
    primitives::BigInt value;
    primitives::Bigint gas_price;
    primitives::BigInt gas_limit;
    uint64_t method;
    common::Buffer params;
  };
}  // namespace fc::primitives::chain

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_HPP
