/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "proofs/proofs_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::proofs, ProofsError, e) {
  using fc::proofs::ProofsError;

  switch (e) {
    case (ProofsError::UNKNOWN):
      return "Proofs: unknown error";
  }
}
