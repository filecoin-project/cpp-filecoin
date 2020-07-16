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
