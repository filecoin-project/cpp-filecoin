/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "filecoin/vm/message/message_util.hpp"

#include "filecoin/codec/cbor/cbor.hpp"
#include "filecoin/crypto/signature/signature.hpp"

namespace fc::vm::message {

  using codec::cbor::encode;
  using common::getCidOf;
  using crypto::signature::typeCode;

  outcome::result<CID> cid(const UnsignedMessage &m) {
    OUTCOME_TRY(encoded, encode<UnsignedMessage>(m));
    return getCidOf(encoded);
  }

  outcome::result<CID> cid(const SignedMessage &sm) {
    if (typeCode(sm.signature) == crypto::signature::Type::BLS) {
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
