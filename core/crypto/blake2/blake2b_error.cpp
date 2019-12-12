/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/blake2/blake2b_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::blake2b, Blake2bError, e) {
  using fc::crypto::blake2b::Blake2bError;

  switch (e) {
    case Blake2bError::OK:
      return "Blake2b: success";
    case Blake2bError::CANNOT_INIT:
      return "Blake2b: cannot init";
    case Blake2bError::UNKNOWN:
      break;
  }

  return "unknown error";
}
