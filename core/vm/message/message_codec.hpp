/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP

#include "codec/cbor/cbor.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {

  CBOR_ENCODE(UnsignedMessage, m) {
    return s << (s.list() << m.to << m.from << m.nonce << m.value << m.gasPrice
                          << m.gasLimit << m.method << m.params);
  }

  CBOR_DECODE(UnsignedMessage, m) {
    s.list() >> m.to >> m.from >> m.nonce >> m.value >> m.gasPrice >> m.gasLimit
        >> m.method >> m.params;
    return s;
  }

  CBOR_ENCODE(SignedMessage, sm) {
    return s << (s.list() << sm.message << sm.signature);
  }

  CBOR_DECODE(SignedMessage, sm) {
    s.list() >> sm.message >> sm.signature;
    return s;
  }

};  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
