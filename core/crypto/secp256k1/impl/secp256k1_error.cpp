/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::secp256k1, Secp256k1Error, e) {
  using fc::crypto::secp256k1::Secp256k1Error;
  switch (e) {
    case Secp256k1Error::kKeyGenerationFailed:
      return "Secp256k1ProviderError: generation failed";
    case Secp256k1Error::kSignatureParseError:
      return "Secp256k1ProviderError: signature parsing error";
    case Secp256k1Error::kSignatureSerializationError:
      return "Secp256k1ProviderError: signature serialization error";
    case Secp256k1Error::kCannotSignError:
      return "Secp256k1ProviderError: error when signing";
    case Secp256k1Error::kPubkeyParseError:
      return "Secp256k1ProviderError: public key parse error";
    case Secp256k1Error::kPubkeySerializationError:
      return "Secp256k1ProviderError: public key serialization error";
    case Secp256k1Error::kRecoverError:
      return "Secp256k1ProviderError: recover error";

    default:
      return "Secp256k1ProviderError: unknown error";
  }
}
