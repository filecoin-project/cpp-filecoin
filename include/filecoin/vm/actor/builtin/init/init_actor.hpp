/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP

#include "filecoin/codec/cbor/streams_annotation.hpp"
#include "filecoin/storage/ipfs/datastore.hpp"
#include "filecoin/vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::init {

  using storage::ipfs::IpfsDatastore;

  /// Init actor state
  struct InitActorState {
    /// Allocate new id address
    outcome::result<Address> addActor(std::shared_ptr<IpfsDatastore> store,
                                      const Address &address);

    CID address_map{};
    uint64_t next_id{};
  };
  CBOR_TUPLE(InitActorState, address_map, next_id)

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

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
