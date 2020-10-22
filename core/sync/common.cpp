/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.hpp"

namespace fc::sync {}  // namespace fc::sync

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
    case E::BRANCHES_LOAD_ERROR:
      return "branches: load error";

    case E::BRANCHES_NO_GENESIS_BRANCH:
      return "branches: no genesis branch";
    case E::BRANCHES_PARENT_EXPECTED:
      return "branches: parent expected";
    case E::BRANCHES_NO_CURRENT_CHAIN:
      return "branches: no current chain";
    case E::BRANCHES_BRANCH_NOT_FOUND:
      return "branches: branch not found";
    case E::BRANCHES_HEAD_NOT_FOUND:
      return "branches: head not found";
    case E::BRANCHES_HEAD_NOT_SYNCED:
      return "branches: head not synced";
    case E::BRANCHES_CYCLE_DETECTED:
      return "branches: cycle detected";
    case E::BRANCHES_STORE_ERROR:
      return "branches: store error";
    case E::BRANCHES_HEIGHT_MISMATCH:
      return "branches: height mismatch";
    case E::BRANCHES_NO_COMMON_ROOT:
      return "branches: no common root";
    case E::BRANCHES_NO_ROUTE:
      return "branches: no route";

    case E::INDEXDB_CANNOT_CREATE:
      return "index db: cannot create";
    case E::INDEXDB_ALREADY_EXISTS:
      return "index db: already exists";
    case E::INDEXDB_EXECUTE_ERROR:
      return "index db: execute error";
    case E::INDEXDB_TIPSET_NOT_FOUND:
      return "index db: tipset not found";

    default:
      break;
  }
  return "sync::Error: unknown error";
}
