/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/state/state_tree.hpp"

namespace fc::vm::actor::builtin::states {
  using state::StateTree;
  using StateTreePtr = std::shared_ptr<StateTree>;
  using primitives::address::Address;

  class StateManager {
   public:
    StateManager(const IpldPtr &ipld,
                 StateTreePtr state_tree,
                 Address receiver);
    ~StateManager() = default;

    AccountActorStatePtr createAccountActorState(ActorVersion version) const;

    outcome::result<AccountActorStatePtr> getAccountActorState() const;

    outcome::result<void> commitState(const std::shared_ptr<State> &state);

   private:
    template <typename T, typename Tv0, typename Tv2, typename Tv3>
    std::shared_ptr<T> createStatePtr(ActorVersion version) const {
      switch (version) {
        case ActorVersion::kVersion0:
          return std::make_shared<Tv0>();
        case ActorVersion::kVersion2:
          return std::make_shared<Tv2>();
        case ActorVersion::kVersion3:
          return std::make_shared<Tv3>();
      }
    }

    template <typename T>
    outcome::result<void> commitCborState(const T &state) {
      OUTCOME_TRY(state_cid, ipld->setCbor(state));
      OUTCOME_TRY(commit(state_cid));
      return outcome::success();
    }

    template <typename Tv0, typename Tv2, typename Tv3>
    outcome::result<void> commitVersionState(
        const std::shared_ptr<State> &state) {
      switch (state->version) {
        case ActorVersion::kVersion0:
          return commitCborState(static_cast<const Tv0 &>(*state));
        case ActorVersion::kVersion2:
          return commitCborState(static_cast<const Tv2 &>(*state));
        case ActorVersion::kVersion3:
          return commitCborState(static_cast<const Tv3 &>(*state));
      }
    }

    outcome::result<void> commit(const CID &new_state);

    IpldPtr ipld;
    StateTreePtr state_tree;
    Address receiver;
    StateProvider provider;
  };
}  // namespace fc::vm::actor::builtin::states
