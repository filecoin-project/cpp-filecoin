/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "vm/message/message_util.hpp"

#include "codec/cbor/cbor.hpp"
#include "crypto/signature/signature.hpp"

namespace fc::vm::message {

  using codec::cbor::encode;
  using common::getCidOf;

  outcome::result<CID> cid(const UnsignedMessage &m) {
    OUTCOME_TRY(encoded, encode<UnsignedMessage>(m));
    return getCidOf(encoded);
  }

  outcome::result<CID> cid(const SignedMessage &sm) {
    if (sm.signature.isBls()) {
      return cid(sm.message);
    }

    OUTCOME_TRY(encoded, encode<SignedMessage>(sm));
    return getCidOf(encoded);
  }

  outcome::result<uint64_t> size(const SignedMessage &sm) {
    OUTCOME_TRY(encoded, encode<SignedMessage>(sm));
    return encoded.size();
  }

};  // namespace fc::vm::message
