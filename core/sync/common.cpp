/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.hpp"

namespace fc::sync {

}  // namespace fc::sync

OUTCOME_CPP_DEFINE_CATEGORY(fc::sync, Error, e) {
  using E = fc::sync::Error;

  switch (e) {
    case E::SYNC_NOT_INITIALIZED:
      return "sync: not initialized";
    case E::SYNC_DATA_INTEGRITY_ERROR:
      return "sync: data integrity error";
    case E::SYNC_UNEXPECTED_OBJECT_STATE:
      return "sync: unexpected object state";
    case E::SYNC_NO_PEERS:
      return "sync: no peers";
    case E::SYNC_BAD_TIPSET:
      return "sync: bad tipset";
    case E::SYNC_BAD_BLOCK:
      return "sync: bad block";
    case E::SYNC_PUBSUB_FAILURE:
      return "sync: pubsub failure";
    case E::SYNC_MSG_LOAD_FAILURE:
      return "sync: block messages load failure";
    case E::SYNC_INCONSISTENT_BLOCKSYNC_RESPONSE:
      return "sync: inconsistent blocksync response";
    case E::SYNC_INCOMPLETE_BLOCKSYNC_RESPONSE:
      return "sync: incomplete blocksync response";
    case E::SYNC_BLOCKSYNC_RESPONSE_ERROR:
      return "sync: blocksync response error";
    default:
      break;
  }
  return "sync::Error: unknown error";
}
