/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP

#include "storage/buffer_map.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::interpreter {
  using storage::PersistentBufferMap;
  using vm::runtime::MessageReceipt;

  class InterpreterImpl : public Interpreter {
   public:
    outcome::result<Result> interpret(const IpldPtr &store,
                                      const Tipset &tipset) const override;
    outcome::result<Result> applyBlocks(
        const IpldPtr &store,
        const Tipset &tipset,
        std::vector<MessageReceipt> *all_receipts) const;

   protected:
    using BlockHeader = primitives::block::BlockHeader;

   private:
    bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) const;
  };

  class CachedInterpreter : public Interpreter {
   public:
    CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                      std::shared_ptr<PersistentBufferMap> store)
        : interpreter{std::move(interpreter)}, store{std::move(store)} {}
    outcome::result<Result> interpret(const IpldPtr &store,
                                      const Tipset &tipset) const override;

   private:
    std::shared_ptr<Interpreter> interpreter;
    std::shared_ptr<PersistentBufferMap> store;
  };
}  // namespace fc::vm::interpreter

#endif
