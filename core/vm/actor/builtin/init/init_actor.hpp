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

  CBOR_ENCODE(InitActorState, state) {
    return s << (s.list() << state.address_map << state.next_id);
  }

  CBOR_DECODE(InitActorState, state) {
    s.list() >> state.address_map >> state.next_id;
    return s;
  }

  constexpr MethodNumber kExecMethodNumber{2};

  struct ExecParams {
    CodeId code;
    MethodParams params;
  };

  outcome::result<InvocationOutput> exec(const Actor &actor,
                                         Runtime &runtime,
                                         const MethodParams &params);

  extern const ActorExports exports;

  CBOR_ENCODE(ExecParams, params) {
    return s << (s.list() << params.code << params.params);
  }

  CBOR_DECODE(ExecParams, params) {
    s.list() >> params.code >> params.params;
    return s;
  }
}  // namespace fc::vm::actor::builtin::init

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
