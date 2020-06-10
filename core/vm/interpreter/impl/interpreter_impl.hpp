/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP

#include "vm/interpreter/interpreter.hpp"

namespace fc::vm::interpreter {
  class InterpreterImpl : public Interpreter {
   public:
    outcome::result<Result> interpret(const IpldPtr &store,
                                      const Tipset &tipset) const override;

   protected:
    using BlockHeader = primitives::block::BlockHeader;

   private:
    bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) const;
  };
}  // namespace fc::vm::interpreter

#endif
