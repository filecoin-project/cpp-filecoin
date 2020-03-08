/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "filecoin/primitives/tipset/tipset.hpp"
#include "filecoin/storage/ipfs/datastore.hpp"
#include "filecoin/vm/indices/indices.hpp"

namespace fc::vm::interpreter {
  enum class InterpreterError {
    DUPLICATE_MINER,
    MINER_SUBMIT_FAILED,
    CRON_TICK_FAILED,
  };

  struct Result {
    CID state_root;
    CID message_receipts;
  };

  class Interpreter {
   protected:
    using Indices = indices::Indices;
    using Tipset = primitives::tipset::Tipset;
    using IpfsDatastore = storage::ipfs::IpfsDatastore;

   public:
    virtual ~Interpreter() = default;

    virtual outcome::result<Result> interpret(
        const std::shared_ptr<IpfsDatastore> &store,
        const Tipset &tipset,
        const std::shared_ptr<Indices> &indices) const = 0;
  };

}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
