/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP

#include <libp2p/multi/content_identifier.hpp>

namespace fc::vm::actor {
  /** Init actor state */
  struct InitActorState {
    ContentIdentifier address_map{
        {}, {}, libp2p::multi::Multihash::create({}, {}).value()};
    uint64_t next_id;
  };

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const InitActorState &state) {
    return s << (s.list() << state.address_map << state.next_id);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, InitActorState &state) {
    s.list() >> state.address_map >> state.next_id;
    return s;
  }
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_INIT_ACTOR_HPP
