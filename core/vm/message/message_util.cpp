/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/message/message_util.hpp"
#include "storage/hamt/hamt.hpp"

namespace fc::vm::message {

  using common::getCidOf;

  outcome::result<CID> cid(const UnsignedMessage &m) {
    auto encoded = serialize<UnsignedMessage>(m);
    return getCidOf(encoded);
  }

  outcome::result<CID> cid(const SignedMessage &sm) {
    if (sm.signature.type == Signature::BLS) {
      return cid(sm.message);
    }

    auto encoded = serialize<SignedMessage>(sm);
    return getCidOf(encoded);
  }

};  // namespace fc::vm::message