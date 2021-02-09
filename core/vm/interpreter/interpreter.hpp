/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "fwd.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/buffer_map.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::vm::interpreter {
  using primitives::BigInt;

  enum class InterpreterError {
    kDuplicateMiner = 1,
    kTipsetMarkedBad,
    kChainInconsistency,
    kNotCached,
  };

  struct Result {
    CID state_root;
    CID message_receipts;
    BigInt weight;
  };
  CBOR_TUPLE(Result, state_root, message_receipts)

  class Interpreter {
   protected:
    using TipsetCPtr = primitives::tipset::TipsetCPtr;

   public:
    virtual ~Interpreter() = default;

    virtual outcome::result<Result> interpret(
        TsBranchPtr ts_branch,
        const IpldPtr &store,
        const TipsetCPtr &tipset) const = 0;
    virtual outcome::result<boost::optional<Result>> tryGetCached(
        const TipsetKey &tsk) const;
    outcome::result<Result> getCached(const TipsetKey &tsk) const;
  };
}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
