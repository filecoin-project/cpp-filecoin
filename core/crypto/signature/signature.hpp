/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
#define CPP_FILECOIN_CRYPTO_SIGNATURE_HPP

#include <boost/variant.hpp>

#include "codec/cbor/cbor.hpp"
#include "common/outcome_throw.hpp"
#include "common/visitor.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"

namespace fc::crypto::signature {
  using BlsSignature = bls::Signature;
  using Secp256k1Signature = secp256k1::Signature;

  enum Type : uint8_t { SECP256K1 = 0x1, BLS = 0x2 };

  constexpr uint64_t kSignatureMaxLength = 200;

  /**
   * @brief Signature error codes
   */
  enum class SignatureError {
    INVALID_SIGNATURE_LENGTH = 1,
    WRONG_SIGNATURE_TYPE,
    INVALID_KEY_LENGTH
  };

  struct Signature : public boost::variant<BlsSignature, Secp256k1Signature> {
    using variant::variant;
    using base_type = boost::variant<BlsSignature, Secp256k1Signature>;

    inline bool operator==(const Signature &other) const {
      return base_type::operator==(static_cast<const base_type &>(other));
    }
  };

  Type typeCode(const Signature &s);

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Signature &signature) {
    std::vector<uint8_t> bytes{};
    visit_in_place(signature,
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

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Signature &signature) {
    std::vector<uint8_t> data{};
    s >> data;
    if (data.empty() || data.size() > kSignatureMaxLength) {
      outcome::raise(SignatureError::INVALID_SIGNATURE_LENGTH);
    }
    switch (data[0]) {
      case (SECP256K1):
        signature = Secp256k1Signature(std::next(data.begin()), data.end());
        break;
      case (BLS): {
        BlsSignature blsSig{};
        if (data.size() != blsSig.size() + 1) {
          outcome::raise(SignatureError::INVALID_SIGNATURE_LENGTH);
        }
        std::copy_n(std::make_move_iterator(std::next(data.begin())),
                    blsSig.size(),
                    blsSig.begin());
        signature = blsSig;
        break;
      }
      default:
        outcome::raise(SignatureError::WRONG_SIGNATURE_TYPE);
    };
    return s;
  }

}  // namespace fc::crypto::signature

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::crypto::signature, SignatureError);

#endif  // CPP_FILECOIN_CRYPTO_SIGNATURE_HPP
