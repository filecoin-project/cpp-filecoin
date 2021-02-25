/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP

#include "blockchain/weight_calculator.hpp"
#include "storage/buffer_map.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/env_context.hpp"
#include "vm/runtime/runtime_randomness.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::interpreter {
  using blockchain::weight::WeightCalculator;
  using runtime::EnvironmentContext;
  using runtime::MessageReceipt;
  using runtime::RuntimeRandomness;
  using storage::PersistentBufferMap;
  using vm::actor::Invoker;
  using vm::runtime::MessageReceipt;

  class InterpreterImpl : public Interpreter {
   public:
    InterpreterImpl(const EnvironmentContext &env_context,
                    std::shared_ptr<WeightCalculator> weight_calculator);

    outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                      const TipsetCPtr &tipset) const override;
    outcome::result<Result> applyBlocks(
        TsBranchPtr ts_branch,
        const TipsetCPtr &tipset,
        std::vector<MessageReceipt> *all_receipts) const;

   protected:
    using BlockHeader = primitives::block::BlockHeader;

   private:
    bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) const;
    outcome::result<BigInt> getWeight(const TipsetCPtr &tipset) const;

    EnvironmentContext env_context_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
  };

  class CachedInterpreter : public Interpreter {
   public:
    CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                      std::shared_ptr<InterpreterCache> cache);
    outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                      const TipsetCPtr &tipset) const override;

   private:
    std::shared_ptr<Interpreter> interpreter;
    std::shared_ptr<InterpreterCache> cache;
  };
}  // namespace fc::vm::interpreter

#endif
