/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
#define CPP_FILECOIN_CRYPTO_SIGNATURE_HPP

#include <boost/variant.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "common/visitor.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"

namespace fc::crypto::signature {
  enum class SignatureError {
    kInvalidSignatureLength = 1,
    kWrongSignatureType,
    kInvalidKeyLength,
  };
}  // namespace fc::crypto::signature

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::signature, SignatureError);

namespace fc::crypto::signature {
  using BlsSignature = bls::Signature;
  using Secp256k1Signature = secp256k1::Signature;

  enum Type : uint8_t { SECP256K1 = 0x1, BLS = 0x2 };

  constexpr uint64_t kSignatureMaxLength = 200;

  struct Signature : public boost::variant<BlsSignature, Secp256k1Signature> {
    using variant::variant;
    using base_type = boost::variant<BlsSignature, Secp256k1Signature>;

    inline bool operator==(const Signature &other) const {
      return base_type::operator==(static_cast<const base_type &>(other));
    }

    inline bool isBls() const {
      return visit_in_place(
          *this,
          [](const BlsSignature &) { return true; },
          [](const auto &) { return false; });
    }

    Buffer toBytes() const;
    static outcome::result<Signature> fromBytes(BytesIn input);
  };

  CBOR_ENCODE(Signature, signature) {
    return s << signature.toBytes();
  }

  CBOR_DECODE(Signature, signature) {
    std::vector<uint8_t> data{};
    s >> data;
    OUTCOME_EXCEPT(sig, Signature::fromBytes(data));
    signature = std::move(sig);
    return s;
  }

}  // namespace fc::crypto::signature

#endif  // CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
