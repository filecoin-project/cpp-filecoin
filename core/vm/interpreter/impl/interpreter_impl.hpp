/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "blockchain/weight_calculator.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/env_context.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::interpreter {
  using blockchain::block_validator::BlockValidator;
  using blockchain::weight::WeightCalculator;
  using primitives::block::BlockHeader;
  using runtime::EnvironmentContext;
  using runtime::MessageReceipt;

  class InterpreterImpl : public Interpreter {
   public:
    InterpreterImpl(EnvironmentContext env_context,
                    std::shared_ptr<BlockValidator> validator,
                    std::shared_ptr<WeightCalculator> weight_calculator);

    outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                      const TipsetCPtr &tipset) const override;
    outcome::result<Result> applyBlocks(
        TsBranchPtr ts_branch,
        const TipsetCPtr &tipset,
        std::vector<MessageReceipt> *all_receipts) const;

   private:
    static bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks);
    outcome::result<BigInt> getWeight(const TipsetCPtr &tipset) const;

    EnvironmentContext env_context_;
    std::shared_ptr<BlockValidator> validator_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
  };

}  // namespace fc::vm::interpreter
