/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::ipfs::graphsync, Error, e) {
  using E = fc::storage::ipfs::graphsync::Error;
  switch (e) {
    case E::MESSAGE_PARSE_ERROR:
      return "message parse error";
    case E::MESSAGE_SIZE_OUT_OF_BOUNDS:
      return "message size out of bounds";
    case E::MESSAGE_VALIDATION_FAILED:
      return "message validation failed";
    default:
      break;
  }
  return "unknown error";
}
