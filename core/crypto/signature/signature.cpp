/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/signature/signature.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::signature, SignatureError, e) {
  using fc::crypto::signature::SignatureError;
  switch (e) {
    case (SignatureError::INVALID_SIGNATURE_LENGTH):
      return "SignatureError: invalid signature length";
    case (SignatureError::WRONG_SIGNATURE_TYPE):
      return "SignatureError: wrong signature type";
    case (SignatureError::INVALID_KEY_LENGTH):
      return "SignatureError: invalid key length";
    default:
      return "SignatureError: unknown error";
  };
}

namespace fc::crypto::signature {

  Type typeCode(const Signature &s) {
    return visit_in_place(
        s,
        [](const BlsSignature &v) { return Type::BLS; },
        [](const Secp256k1Signature &v) { return Type::SECP256K1; });
  }

}  // namespace fc::crypto::signature
