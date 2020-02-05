/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/amt/amt.hpp"

#include "common/which.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::amt, AmtError, e) {
  using fc::storage::amt::AmtError;
  switch (e) {
    case AmtError::EXPECTED_CID:
      return "Expected CID";
    case AmtError::DECODE_WRONG:
      return "Decode wrong";
  }
  return "Unknown error";
}
