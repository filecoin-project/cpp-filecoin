/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "primitives/tipset/tipset.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/indices/indices.hpp"

namespace fc::vm::interpreter {
  enum class InterpreterError {
    DUPLICATE_MINER,
    MINER_SUBMIT_FAILED,
    CRON_TICK_FAILED,
  };

  using indices::Indices;
  using primitives::tipset::Tipset;
  using storage::ipfs::IpfsDatastore;

  struct Result {
    CID state_root;
    CID message_receipts;
  };

  outcome::result<Result> interpret(const std::shared_ptr<IpfsDatastore> &store,
                                    const Tipset &tipset,
                                    const std::shared_ptr<Indices> &indices);
}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
