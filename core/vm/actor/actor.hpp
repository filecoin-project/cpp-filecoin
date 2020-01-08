/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP

#include <libp2p/multi/content_identifier_codec.hpp>

#include "common/cid.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::actor {
  using libp2p::multi::ContentIdentifier;
  using primitives::address::Address;
  using primitives::BigInt;

  /** Common actor state interface */
  struct Actor {
    ContentIdentifier code{common::kEmptyCid};
    ContentIdentifier head{common::kEmptyCid};
    uint64_t nonce{};
    BigInt balance;
  };

  bool operator==(const Actor &lhs, const Actor &rhs);

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Actor &actor) {
    return s << (s.list() << actor.code << actor.head << actor.nonce << actor.balance);
  }

  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
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

  inline static const auto kInitAddress = Address::makeFromId(0);
  inline static const auto kStoragePowerAddress = Address::makeFromId(2);
  inline static const auto kCronAddress = Address::makeFromId(4);
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
