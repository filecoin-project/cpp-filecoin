/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP

#include <libp2p/multi/content_identifier_codec.hpp>

#include "primitives/big_int.hpp"

namespace fc::vm::actor {
  using primitives::BigInt;
  using libp2p::multi::ContentIdentifier;

  /** Common actor state interface */
  struct Actor {
    ContentIdentifier code;
    ContentIdentifier head;
    uint64_t nonce{};
    BigInt balance;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Actor &actor) {
    return s << (s.list() << actor.code << actor.head << actor.nonce << actor.balance);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Actor &actor) {
    s.list() >> actor.code >> actor.head >> actor.nonce >> actor.balance;
    return s;
  }

  /** Check if code specifies builtin actor implementation */
  bool isBuiltinActor(const ContentIdentifier &code);

  /** Check if only one instance of actor should exists */
  bool isSingletonActor(const ContentIdentifier &code);

  extern const ContentIdentifier kEmptyObjectCid;

  extern const ContentIdentifier kAccountCodeCid, kCronCodeCid, kStoragePowerCodeCid,
      kStorageMarketCodeCid, kStorageMinerCodeCid, kMultisigCodeCid,
      kInitCodeCid, kPaymentChannelCodeCid;
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
