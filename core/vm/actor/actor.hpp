/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP

#include <boost/serialization/strong_typedef.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>

#include "common/buffer.hpp"
#include "primitives/big_int.hpp"

namespace fc::vm::actor {

  using libp2p::multi::ContentIdentifier;
  using primitives::BigInt;
  using Serialization = fc::common::Buffer;

  /**
   * Amount of Filecoin tokens
   */
  class TokenAmount : public BigInt {};

  /**
   * Consider MethodNum numbers to be similar in concerns as for offsets in
   * function tables (in programming languages), and for tags in ProtocolBuffer
   * fields. Tags in ProtocolBuffers recommend assigning a unique tag to a field
   * and never reusing that tag. If a field is no longer used, the field name
   * may change but should still remain defined in the code to ensure the tag
   * number is not reused accidentally. The same should apply to the MethodNum
   * associated with methods in Filecoin VM Actors.
   */
  struct MethodNumber {
    int method_number;
  };

  /**
   * MethodParams is an array of objects to pass into a method. This is the list
   * of arguments/parameters
   */
  class MethodParams : public gsl::span<Serialization> {};
  using primitives::BigInt;
  using Serialization = fc::common::Buffer;

  /**
   * CodeID identifies an actor's code (either one of the builtin actors, or, in
   * the future, potentially a CID of VM code for a custom actor.)
   */
  class CodeId : public ContentIdentifier {
   public:
    explicit CodeId(ContentIdentifier cid)
        : ContentIdentifier{std::move(cid)} {}
  };

  class ActorSubstateCID : public ContentIdentifier {
   public:
    explicit ActorSubstateCID(ContentIdentifier cid)
        : ContentIdentifier{std::move(cid)} {}
  };

  /**
   * Common actor state interface represents the on-chain storage all actors
   * keep
   */
  struct Actor {
    /// Identifies the code this actor executes
    CodeId code;
    /// CID of the root of optional actor-specific sub-state
    ContentIdentifier head;
    /// Expected sequence number of the next message sent by this actor
    uint64_t nonce{};
    /// Balance of tokens held by this actor
    TokenAmount balance;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Actor &actor) {
    return s << (s.list() << actor.code << actor.head << actor.nonce
                          << actor.balance);
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Actor &actor) {
    s.list() >> actor.code >> actor.head >> actor.nonce >> actor.balance;
    return s;
  }

  /** Check if code specifies builtin actor implementation */
  bool isBuiltinActor(const CodeId &code);

  /** Check if only one instance of actor should exists */
  bool isSingletonActor(const CodeId &code);

  extern const ContentIdentifier kEmptyObjectCid;

  extern const CodeId kAccountCodeCid, kCronCodeCid, kStoragePowerCodeCid,
      kStorageMarketCodeCid, kStorageMinerCodeCid, kMultisigCodeCid,
      kInitCodeCid, kPaymentChannelCodeCid;

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
