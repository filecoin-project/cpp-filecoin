/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP

#include "codec/cbor/cbor.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {

  using fc::codec::cbor::CborEncodeStream;

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const UnsignedMessage &m) {
    return s << (s.list() << m.to << m.from << m.nonce << m.value << m.gasPrice
                          << m.gasLimit << m.method << m.params);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, UnsignedMessage &m) {
    s.list() >> m.to >> m.from >> m.nonce >> m.value >> m.gasPrice >> m.gasLimit
        >> m.method >> m.params;
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &s, const SignedMessage &sm) {
    return s << (s.list() << sm.message << sm.signature);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &s, SignedMessage &sm) {
    s.list() >> sm.message >> sm.signature;
    return s;
  }

};  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
