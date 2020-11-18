/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP

#include "primitives/tipset/tipset.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/buffer_map.hpp"

namespace fc::vm::interpreter {
  enum class InterpreterError {
    kDuplicateMiner = 1,
    kMinerSubmitFailed,
    kCronTickFailed,
    kTipsetMarkedBad,
    kChainInconsistency,
  };

  struct Result {
    CID state_root;
    CID message_receipts;
  };
  CBOR_TUPLE(Result, state_root, message_receipts)

  class Interpreter {
   protected:
    using TipsetCPtr = primitives::tipset::TipsetCPtr;

   public:
    virtual ~Interpreter() = default;

    virtual outcome::result<Result> interpret(
        const IpldPtr &store, const TipsetCPtr &tipset) const = 0;
  };

  /// returns persisted interpreter result for tipset, if exists,
  /// empty value if tipset is not yet interpreted,
  /// error if tipset is bad or store access error occured
  outcome::result<boost::optional<Result>> getSavedResult(
      const storage::PersistentBufferMap &store,
      const primitives::tipset::TipsetCPtr &tipset);

}  // namespace fc::vm::interpreter

OUTCOME_HPP_DECLARE_ERROR(fc::vm::interpreter, InterpreterError);

#endif  // CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_HPP
