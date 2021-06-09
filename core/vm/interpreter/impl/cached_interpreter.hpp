/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "vm/interpreter/interpreter.hpp"

namespace fc::vm::interpreter {

  class CachedInterpreter : public Interpreter {
   public:
    CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                      std::shared_ptr<InterpreterCache> cache);
    outcome::result<Result> interpret(TsBranchPtr ts_branch,
                                      const TipsetCPtr &tipset) const override;

   private:
    std::shared_ptr<Interpreter> interpreter;
    std::shared_ptr<InterpreterCache> cache;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::vm::interpreter::CachedInterpreter);
  };

}  // namespace fc::vm::interpreter
