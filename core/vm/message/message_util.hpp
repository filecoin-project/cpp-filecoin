/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_UTIL_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_UTIL_HPP

#include "primitives/cid/cid.hpp"
#include "vm/message/message_codec.hpp"

namespace fc::vm::message {

  /**
   * @brief Extracts the canonical (CBOR-encoded) CID of an UnsignedMessage
   */
  outcome::result<CID> cid(const UnsignedMessage &m);

  /**
   * @brief Extracts the canonical (CBOR-encoded) CID of a SignedMessage
   */
  outcome::result<CID> cid(const SignedMessage &sm);

  /**
   * @brief SignedMessage size
   */
  outcome::result<uint64_t> size(const SignedMessage &sm);

};  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_UTIL_HPP
