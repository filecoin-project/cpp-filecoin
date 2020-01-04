/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_RECEIPT_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_RECEIPT_HPP

#include "common/buffer.hpp"
#include "primitives/big_int.hpp"

// TODO (yuraz): FIL-72 not really used yet, just a sketch remove this message when
// real structures  implemented

namespace fc::primitives::chain {
  /**
   * @brief MessageReceipt struct for chain::types
   */
  struct MessageReceipt {
    uint8_t exit_code;
    common::Buffer return_value;
    primitives::BigInt gas_used;
  };

  inline bool operator==(const MessageReceipt &lhs, const MessageReceipt &rhs) {
    return lhs.exit_code == rhs.exit_code
           && lhs.return_value == rhs.return_value
           && lhs.gas_used == rhs.gas_used;
  }
}  // namespace fc::primitives::chain

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CHAIN_MESSAGE_RECEIPT_HPP
