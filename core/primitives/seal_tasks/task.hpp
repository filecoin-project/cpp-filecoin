/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

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
}  // namespace fc::primitives
