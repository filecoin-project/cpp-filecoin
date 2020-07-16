/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
#define CPP_FILECOIN_CRYPTO_SIGNATURE_HPP

#include <boost/variant.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
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
  };

  CBOR_ENCODE(Signature, signature) {
    std::vector<uint8_t> bytes{};
    visit_in_place(
        signature,
        [&bytes](const BlsSignature &v) {
          bytes.push_back(BLS);
          return bytes.insert(bytes.end(), v.begin(), v.end());
        },
        [&bytes](const Secp256k1Signature &v) {
          bytes.push_back(SECP256K1);
          return bytes.insert(bytes.end(), v.begin(), v.end());
        });
    return s << bytes;
  }

  CBOR_DECODE(Signature, signature) {
    std::vector<uint8_t> data{};
    s >> data;
    if (data.empty() || data.size() > kSignatureMaxLength) {
      outcome::raise(SignatureError::kInvalidSignatureLength);
    }
    switch (data[0]) {
      case (SECP256K1): {
        Secp256k1Signature secp256K1Signature{};
        if (data.size() != secp256K1Signature.size() + 1) {
          outcome::raise(SignatureError::kInvalidSignatureLength);
        }
        std::copy_n(std::make_move_iterator(std::next(data.begin())),
                    secp256K1Signature.size(),
                    secp256K1Signature.begin());
        signature = secp256K1Signature;
        break;
      }
      case (BLS): {
        BlsSignature blsSig{};
        if (data.size() != blsSig.size() + 1) {
          outcome::raise(SignatureError::kInvalidSignatureLength);
        }
        std::copy_n(std::make_move_iterator(std::next(data.begin())),
                    blsSig.size(),
                    blsSig.begin());
        signature = blsSig;
        break;
      }
      default:
        outcome::raise(SignatureError::kWrongSignatureType);
    };
    return s;
  }

}  // namespace fc::crypto::signature

#endif  // CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
