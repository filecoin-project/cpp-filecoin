/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_VM_RUNTIME_ENV_HPP
#define FILECOIN_CORE_VM_RUNTIME_ENV_HPP

#include "crypto/randomness/randomness_provider.hpp"
#include "vm/actor/invoker.hpp"
#include "vm/indices/indices.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::runtime {
  using actor::Invoker;
  using crypto::randomness::RandomnessProvider;
  using indices::Indices;
  using state::StateTree;

  /// Environment contains objects that are shared by runtime contexts
  struct Env : std::enable_shared_from_this<Env> {
    Env(std::shared_ptr<RandomnessProvider> randomness_provider,
        std::shared_ptr<StateTree> state_tree,
        std::shared_ptr<Indices> indices,
        std::shared_ptr<Invoker> invoker,
        ChainEpoch chain_epoch,
        Address block_miner)
        : randomness_provider{std::move(randomness_provider)},
          state_tree{std::move(state_tree)},
          indices{std::move(indices)},
          invoker{std::move(invoker)},
          chain_epoch{chain_epoch},
          block_miner{block_miner} {}

    outcome::result<MessageReceipt> applyMessage(
        const UnsignedMessage &message);

    outcome::result<InvocationOutput> send(BigInt &gas_used,
                                           const Address &origin,
                                           const UnsignedMessage &message);

    std::shared_ptr<RandomnessProvider> randomness_provider;
    std::shared_ptr<StateTree> state_tree;
    std::shared_ptr<Indices> indices;
    std::shared_ptr<Invoker> invoker;
    ChainEpoch chain_epoch;
    Address block_miner;
  };
}  // namespace fc::vm::runtime

#endif  // FILECOIN_CORE_VM_RUNTIME_ENV_HPP
