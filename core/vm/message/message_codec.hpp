/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP

#include <vector>

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
#include "common/visitor.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {

  using fc::codec::cbor::CborEncodeStream;

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const UnsignedMessage &m) noexcept {
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
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Signature &signature) noexcept {
    std::vector<uint8_t> bytes{};
    bytes.push_back(signature.type);
    visit_in_place(signature.data, [&bytes](const auto &v) {
      return bytes.insert(bytes.end(), v.begin(), v.end());
    });
    return s << bytes;
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Signature &signature) {
    std::vector<uint8_t> data{};
    s >> data;
    if (data.empty() || data.size() > kSignatureMaxLength) {
      outcome::raise(MessageError::INVALID_SIGNATURE_LENGTH);
    }
    switch (data[0]) {
      case (Signature::SECP256K1):
        signature.type = Signature::SECP256K1;
        signature.data =
            Signature::Secp256k1Signature(std::next(data.begin()), data.end());
        break;
      case (Signature::BLS): {
        signature.type = Signature::BLS;
        if (data.size() != kBlsSignatureLength + 1) {
          outcome::raise(MessageError::INVALID_SIGNATURE_LENGTH);
        }
        Signature::BlsSignature blsSig{};
        std::copy_n(std::make_move_iterator(std::next(data.begin())),
                    kBlsSignatureLength,
                    blsSig.begin());
        signature.data = blsSig;
        break;
      }
      default:
        outcome::raise(MessageError::WRONG_SIGNATURE_TYPE);
    };
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &s, const SignedMessage &sm) noexcept {
    return s << (s.list() << sm.message << sm.signature);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &s, SignedMessage &sm) {
    s.list() >> sm.message >> sm.signature;
    return s;
  }

  /**
   * @brief Encodes a Message (signed or unsigned) to an array of bytes
   */
  template <
      class T,
      typename = std::enable_if_t<
          std::is_same_v<T,
                         UnsignedMessage> || std::is_same_v<T, SignedMessage>>>
  static std::vector<uint8_t> serialize(const T &m) {
    CborEncodeStream encoder;
    encoder << m;
    return encoder.data();
  }

  /**
   * @brief Decodes a Message (signed or unsigned) from an array of bytes
   */
  template <
      class T,
      typename = std::enable_if_t<
          std::is_same_v<T,
                         UnsignedMessage> || std::is_same_v<T, SignedMessage>>>
  static outcome::result<T> decode(const gsl::span<uint8_t> &v) {
    return codec::cbor::decode<T>(v);
  }

};  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_CODEC_HPP
