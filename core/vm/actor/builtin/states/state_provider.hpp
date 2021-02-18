/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"

#include "vm/actor/builtin/states/account_actor_state.hpp"

namespace fc::vm::actor::builtin::states {

  class StateProvider {
   public:
    explicit StateProvider(IpldPtr ipld);
    ~StateProvider() = default;

    outcome::result<AccountActorStatePtr> getAccountActorState(
        const Actor &actor) const;

   private:
    ActorVersion getVersion(const CodeId &code) const;

    template <typename T>
    outcome::result<std::shared_ptr<T>> getStatePtr(const CID &head) const {
      OUTCOME_TRY(state, ipld->getCbor<T>(head));
      return std::make_shared<T>(state);
    }

    template <typename T, typename Tv0, typename Tv2, typename Tv3>
    outcome::result<std::shared_ptr<T>> getCommonStatePtr(
        const Actor &actor) const {
      const auto version = getVersion(actor.code);

      std::shared_ptr<T> result;

      if (version == ActorVersion::kVersion0) {
        OUTCOME_TRY(state0, getStatePtr<Tv0>(actor.head));
        result = std::static_pointer_cast<T>(state0);
      } else if (version == ActorVersion::kVersion2) {
        OUTCOME_TRY(state2, getStatePtr<Tv2>(actor.head));
        result = std::static_pointer_cast<T>(state2);
      } else if (version == ActorVersion::kVersion3) {
        OUTCOME_TRY(state3, getStatePtr<Tv3>(actor.head));
        result = std::static_pointer_cast<T>(state3);
      } else {
        return VMExitCode::kSysErrIllegalArgument;
      }

      return std::move(result);
    }

    IpldPtr ipld;
  };
}  // namespace fc::vm::actor::builtin::states
