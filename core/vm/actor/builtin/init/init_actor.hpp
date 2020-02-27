/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor_method.hpp"

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

  struct ExecParams {
    CodeId code;
    MethodParams params;
  };

  struct ExecReturn {
    Address id_address;      // The canonical ID-based address for the actor
    Address robust_address;  // A more expensive but re-org-safe address for the
                             // newly created actor
  };

  struct Exec : ActorMethodBase<2> {
    using Params = ExecParams;
    using Result = ExecReturn;
    ACTOR_METHOD_STUB();
  };

  extern const ActorExports exports;

  CBOR_TUPLE(ExecParams, code, params)

  CBOR_TUPLE(ExecReturn, id_address, robust_address)

  inline bool operator==(const Exec::Result &lhs, const Exec::Result &rhs) {
    return lhs.id_address == rhs.id_address
           && lhs.robust_address == rhs.robust_address;
  }

}  // namespace fc::vm::actor::builtin::init

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
