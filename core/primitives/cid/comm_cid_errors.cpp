/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/comm_cid_errors.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, CommCidError, e) {
  using fc::common::CommCidError;

  switch (e) {
    case (CommCidError::kInvalidHash):
      return "CommCid: incorrect hashing function for data commitment";
    default:
      return "Piece: unknown error";
  }
}
