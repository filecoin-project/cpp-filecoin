/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_ENV_HPP
#define FILECOIN_CORE_VM_RUNTIME_ENV_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "primitives/types.hpp"
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
        UnsignedMessage message) {
      OUTCOME_TRY(from, state_tree->get(message.from));
      message.nonce = from.nonce;
      GasAmount gas_used{0};
      return send(gas_used, message.from, message);
    }

    outcome::result<InvocationOutput> send(GasAmount &gas_used,
                                           const Address &origin,
                                           const UnsignedMessage &message);

    std::shared_ptr<RandomnessProvider> randomness_provider;
    std::shared_ptr<StateTree> state_tree;
    std::shared_ptr<Invoker> invoker;
    ChainEpoch chain_epoch;
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_ENV_HPP
