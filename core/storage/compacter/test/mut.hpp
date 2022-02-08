#pragma once

#include "common/logger.hpp"
#include "vm/actor/builtin/states/account/account_actor_state.hpp"
#include "vm/actor/codes.hpp"
#include "vm/state/state_tree.hpp"

namespace fc {
  template <typename T>
  auto O(outcome::result<T> &&o) {
    return std::move(o).value();
  }
}  // namespace fc

namespace fc::mut {
  using vm::actor::builtin::states::AccountActorStatePtr;
  using vm::actor::builtin::types::Universal;
  using vm::state::Actor;
  using vm::state::Address;
  using vm::state::StateTree;
  namespace code = vm::actor::builtin::v7;

  const auto addr1{Address::makeFromId(100)};

  template <typename T>
  struct _IsUniversal;
  template <typename T>
  struct _IsUniversal<Universal<T>> {
    using type = T;
  };
  template <typename T>
  auto _universal(const IpldPtr &ipld) {
    Universal<typename _IsUniversal<T>::type> u{ipld->actor_version};
    cbor_blake::cbLoadT(ipld, u);
    return u;
  }
#define MAKE(T) _universal<T>(ipld)
#define PUT(x) O(setCbor(ipld, x))
#define GET(c, T) O(getCbor<T>(ipld, c))

  // set up genesis state
  // EDIT
  inline void genesis(const IpldPtr &ipld,
                      const std::shared_ptr<StateTree> &tree) {
    // TODO: init actor
    // TODO: market actor
    // TODO: power actor
    Actor actor1;
    actor1.code = code::kAccountCodeId;
    actor1.head = PUT(MAKE(AccountActorStatePtr));
    O(tree->set(addr1, actor1));
    spdlog::info("mut::genesis");
  }

  // change state for new blocks
  // EDIT
  inline void block(const IpldPtr &ipld,
                    const std::shared_ptr<StateTree> &tree,
                    ChainEpoch epoch) {
    auto actor1{O(tree->get(addr1))};
    auto state1{GET(actor1.head, AccountActorStatePtr)};
    state1->address = Address::makeFromId(state1->address.getId() + 1);
    actor1.head = PUT(state1);
    O(tree->set(addr1, actor1));
    spdlog::info("mut::block {}", epoch);
  }
}  // namespace fc::mut
