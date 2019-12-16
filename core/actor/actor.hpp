/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP

#include <libp2p/multi/content_identifier_codec.hpp>

#include "primitives/big_int.hpp"

namespace fc::actor {
  using primitives::BigInt;
  using libp2p::multi::ContentIdentifier;

  struct Actor {
    ContentIdentifier code;
    ContentIdentifier head;
    uint64_t nonce;
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

  bool isBuiltinActor(ContentIdentifier code);

  bool isSingletonActor(ContentIdentifier code);

  extern ContentIdentifier kEmptyObjectCid;

  extern ContentIdentifier kAccountCodeCid, kCronCodeCid, kStoragePowerCodeCid,
      kStorageMarketCodeCid, kStorageMinerCodeCid, kMultisigCodeCid,
      kInitCodeCid, kPaymentChannelCodeCid;
}  // namespace fc::actor

#endif  // CPP_FILECOIN_CORE_ACTOR_ACTOR_HPP
