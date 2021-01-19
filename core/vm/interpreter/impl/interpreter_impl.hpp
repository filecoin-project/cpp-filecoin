/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP

#include "storage/buffer_map.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/interpreter/interpreter.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/runtime_randomness.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::interpreter {
  using runtime::MessageReceipt;
  using runtime::RuntimeRandomness;
  using storage::PersistentBufferMap;
  using vm::actor::Invoker;
  using vm::runtime::MessageReceipt;

  class InterpreterImpl : public Interpreter {
   public:
    InterpreterImpl(std::shared_ptr<Invoker> invoker,
                    std::shared_ptr<RuntimeRandomness> randomness,
                    std::shared_ptr<Circulating> circulating);

    outcome::result<Result> interpret(const IpldPtr &store,
                                      const TipsetCPtr &tipset) const override;
    outcome::result<Result> applyBlocks(
        const IpldPtr &store,
        const TipsetCPtr &tipset,
        std::vector<MessageReceipt> *all_receipts) const;

   protected:
    using BlockHeader = primitives::block::BlockHeader;

   private:
    bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) const;

    std::shared_ptr<Invoker> invoker_;
    std::shared_ptr<RuntimeRandomness> randomness_;
    std::shared_ptr<Circulating> circulating_;
  };

  class CachedInterpreter : public Interpreter {
   public:
    CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                      std::shared_ptr<PersistentBufferMap> store)
        : interpreter{std::move(interpreter)}, store{std::move(store)} {}
    outcome::result<Result> interpret(const IpldPtr &store,
                                      const TipsetCPtr &tipset) const override;

   private:
    std::shared_ptr<Interpreter> interpreter;
    std::shared_ptr<PersistentBufferMap> store;
  };
}  // namespace fc::vm::interpreter

#endif
