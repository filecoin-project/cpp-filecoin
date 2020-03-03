/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_INTERPRETER_INTERPRETER_IMPL_HPP

#include "vm/interpreter/interpreter.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm::interpreter {
  class InterpreterImpl : public Interpreter {
   public:
    outcome::result<Result> interpret(
        const std::shared_ptr<IpfsDatastore> &store,
        const Tipset &tipset,
        const std::shared_ptr<Indices> &indices) const override;

   protected:
    using BlockHeader = primitives::block::BlockHeader;
    using Address = primitives::address::Address;
    using StateTreeImpl = vm::state::StateTreeImpl;

   private:
    bool hasDuplicateMiners(const std::vector<BlockHeader> &blocks) const;

    outcome::result<Address> getMinerOwner(StateTreeImpl &state_tree,
                                           const Address &miner) const;
  };
}  // namespace fc::vm::interpreter

#endif
