/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "primitives/tipset/tipset.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::interpreter {
  enum class InterpreterError {
    kDuplicateMiner = 1,
    kMinerSubmitFailed,
    kCronTickFailed,
  };

  struct Result {
    CID state_root;
    CID message_receipts;
  };
  CBOR_TUPLE(Result, state_root, message_receipts)

  class Interpreter {
   protected:
    using Tipset = primitives::tipset::Tipset;

   public:
    virtual ~Interpreter() = default;

    virtual outcome::result<Result> interpret(const IpldPtr &store,
                                              const Tipset &tipset) const = 0;
  };

}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
