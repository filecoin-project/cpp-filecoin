/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/v0/init/init_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::init {

  struct Construct : ActorMethodBase<1> {
    struct Params {
      std::string network_name;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(Construct::Params, network_name)

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

    static bool canExec(const Runtime &runtime,
                        const CID &caller_code_id,
                        const CID &exec_code_id);
  };
  CBOR_TUPLE(Exec::Params, code, params)
  CBOR_TUPLE(Exec::Result, id_address, robust_address)

  inline bool operator==(const Exec::Result &lhs, const Exec::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

  extern const ActorExports exports;

}  // namespace fc::vm::actor::builtin::v0::init

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::init::InitActorState> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::init::InitActorState &state,
                     const Visitor &visit) {
      visit(state.address_map);
    }
  };
}  // namespace fc
