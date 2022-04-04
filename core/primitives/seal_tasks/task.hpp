/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include "common/map_utils.hpp"

namespace fc::primitives {
  using TaskType = std::string;

  const TaskType kTTAddPiece("seal/v0/addpiece");
  const TaskType kTTPreCommit1("seal/v0/precommit/1");
  const TaskType kTTPreCommit2("seal/v0/precommit/2");
  const TaskType kTTCommit1(
      "seal/v0/commit/1");  // NOTE: We use this to transfer the sector into
                            // miner-local storage for now; Don't use on
                            // workers!
  const TaskType kTTCommit2("seal/v0/commit/2");

  const TaskType kTTFinalize("seal/v0/finalize");

  const TaskType kTTFetch("seal/v0/fetch");
  const TaskType kTTUnseal("seal/v0/unseal");
  // TODO (a.chernyshov) ReadUnsealed was removed in Lotus
  // (https://github.com/filecoin-project/cpp-filecoin/issues/574)
  const TaskType kTTReadUnsealed("seal/v0/unsealread");

  const TaskType kTTReplicaUpdate("seal/v0/replicaupdate");
  const TaskType kTTProveReplicaUpdate1("seal/v0/provereplicaupdate/1");
  const TaskType kTTProveReplicaUpdate2("seal/v0/provereplicaupdate/2");
  const TaskType kTTRegenSectorKey("seal/v0/regensectorkey");
  const TaskType kTTFinalizeReplicaUpdate("seal/v0/finalize/replicaupdate");

  /**
   * Returns task type order. If task type not on the list, 0 returned.
   */
  inline int getTaskTypePiority(std::string_view task) {
    static std::unordered_map<std::string_view, int> order = {
        {kTTFinalize, -2},
        {kTTFetch, -1},
        {kTTUnseal, 1},
        {kTTCommit1, 2},
        {kTTCommit2, 3},
        {kTTPreCommit2, 4},
        {kTTPreCommit1, 5},
        {kTTProveReplicaUpdate1, 6},
        {kTTProveReplicaUpdate2, 7},
        {kTTReplicaUpdate, 8},
        {kTTAddPiece, 9},
        {kTTRegenSectorKey, 10}};

    return common::getOrDefault(order, task, 0);
  }
}  // namespace fc::primitives
