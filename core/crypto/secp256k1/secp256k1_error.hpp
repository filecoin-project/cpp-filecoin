/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::crypto::secp256k1 {

  enum class Secp256k1Error {
    kKeyGenerationFailed = 1,
    kSignatureParseError,
    kSignatureSerializationError,
    kCannotSignError,
    kPubkeyParseError,
    kPubkeySerializationError,
    kRecoverError,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::crypto::secp256k1, Secp256k1Error);
