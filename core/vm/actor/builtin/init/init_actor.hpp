/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP

#include "adt/address_key.hpp"
#include "adt/map.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::init {
  /// Init actor state
  struct InitActorState {
    /// Allocate new id address
    outcome::result<Address> addActor(const Address &address);

    adt::Map<uint64_t, adt::AddressKeyer> address_map;
    uint64_t next_id{};
    std::string network_name;
  };
  CBOR_TUPLE(InitActorState, address_map, next_id, network_name)

  struct Exec : ActorMethodBase<2> {
    struct Params {
      CodeId code;
      MethodParams params;
    };
    struct Result {
      Address id_address;      // The canonical ID-based address for the actor
      Address robust_address;  // A more expensive but re-org-safe address for
                               // the newly created actor
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Exec::Params, code, params)
  CBOR_TUPLE(Exec::Result, id_address, robust_address)

  inline bool operator==(const Exec::Result &lhs, const Exec::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::init

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::init::InitActorState> {
    template <typename F>
    static void f(vm::actor::builtin::init::InitActorState &s, const F &f) {
      f(s.address_map);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
