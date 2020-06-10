/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_ENV_HPP
#define FILECOIN_CORE_VM_RUNTIME_ENV_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "primitives/types.hpp"
#include "storage/hamt/hamt.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using crypto::randomness::RandomnessProvider;
  using state::StateTree;

  /// Environment contains objects that are shared by runtime contexts
  struct Env : std::enable_shared_from_this<Env> {
    Env(std::shared_ptr<RandomnessProvider> randomness_provider,
        std::shared_ptr<StateTree> state_tree,
        std::shared_ptr<Invoker> invoker,
        ChainEpoch chain_epoch)
        : randomness_provider{std::move(randomness_provider)},
          state_tree{std::move(state_tree)},
          invoker{std::move(invoker)},
          chain_epoch{chain_epoch} {}

    outcome::result<MessageReceipt> applyMessage(const UnsignedMessage &message,
                                                 TokenAmount &penalty);

    outcome::result<InvocationOutput> applyImplicitMessage(
        UnsignedMessage message);

    std::shared_ptr<RandomnessProvider> randomness_provider;
    std::shared_ptr<StateTree> state_tree;
    std::shared_ptr<Invoker> invoker;
    ChainEpoch chain_epoch;
  };

  struct Execution : std::enable_shared_from_this<Execution> {
    static std::shared_ptr<Execution> make(std::shared_ptr<Env> env,
                                           const UnsignedMessage &message);

    outcome::result<void> chargeGas(GasAmount amount);

    outcome::result<Actor> tryCreateAccountActor(const Address &address);

    outcome::result<InvocationOutput> sendWithRevert(
        const UnsignedMessage &message);

    outcome::result<InvocationOutput> send(const UnsignedMessage &message);

    std::shared_ptr<Env> env;
    std::shared_ptr<StateTree> state_tree;
    GasAmount gas_used;
    GasAmount gas_limit;
    Address origin;
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_ENV_HPP
