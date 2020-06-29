/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_SEAL_TASKS_TASK_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_SEAL_TASKS_TASK_HPP

#include <string>
#include <unordered_map>

namespace fc::primitives {

  class TaskType : public std::string {
   public:
    explicit TaskType(const std::string &str) : std::string(str){};
  };

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
  const TaskType kTTReadUnsealed("seal/v0/unsealread");

  inline bool operator<(const TaskType &lhs, const TaskType &rhs) {
    static std::unordered_map<std::string, int> order = {{kTTReadUnsealed, 0},
                                                         {kTTUnseal, 0},
                                                         {kTTFinalize, 1},
                                                         {kTTFetch, 2},
                                                         {kTTCommit1, 3},
                                                         {kTTCommit2, 4},
                                                         {kTTPreCommit2, 5},
                                                         {kTTPreCommit1, 6},
                                                         {kTTAddPiece, 7}};

    return order[lhs] < order[rhs];
  }

}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_SEAL_TASKS_TASK_HPP
