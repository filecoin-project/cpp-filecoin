/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/signature/signature.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::signature, SignatureError, e) {
  using fc::crypto::signature::SignatureError;
  switch (e) {
    case (SignatureError::kInvalidSignatureLength):
      return "SignatureError: invalid signature length";
    case (SignatureError::kWrongSignatureType):
      return "SignatureError: wrong signature type";
    case (SignatureError::kInvalidKeyLength):
      return "SignatureError: invalid key length";
    default:
      return "SignatureError: unknown error";
  };
}

namespace fc::crypto::signature {
  Buffer Signature::toBytes() const {
    Buffer bytes;
    visit_in_place(
        *this,
        [&](const BlsSignature &v) {
          bytes.put({kBls});
          bytes.put(v);
        },
        [&](const Secp256k1Signature &v) {
          bytes.put({kSecp256k1});
          bytes.put(v);
        });
    return bytes;
  }

  outcome::result<Signature> Signature::fromBytes(BytesIn input) {
    if (input.empty() || (size_t)input.size() > kSignatureMaxLength) {
      return SignatureError::kInvalidSignatureLength;
    }
    switch (input[0]) {
      case kSecp256k1: {
        Secp256k1Signature secp{};
        if (input.size() != secp.size() + 1) {
          outcome::raise(SignatureError::kInvalidSignatureLength);
        }
        std::copy_n(std::make_move_iterator(std::next(input.begin())),
                    secp.size(),
                    secp.begin());
        return secp;
      }
      case kBls: {
        BlsSignature bls;
        if (input.size() != bls.size() + 1) {
          return SignatureError::kInvalidSignatureLength;
        }
        std::copy_n(std::make_move_iterator(std::next(input.begin())),
                    bls.size(),
                    bls.begin());
        return bls;
      }
    }
    return SignatureError::kWrongSignatureType;
  }

  outcome::result<bool> Signature::isBls(const BytesIn &input) {
    if (input.empty()) {
      return SignatureError::kWrongSignatureType;
    }
    switch (input[0]) {
      case kSecp256k1:
        return false;
      case kBls:
        return true;
    }
    return SignatureError::kWrongSignatureType;
  }
}  // namespace fc::crypto::signature
