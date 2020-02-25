/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP

#include <boost/operators.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::vm::actor {

  using fc::common::Buffer;
  using primitives::BigInt;
  using primitives::address::Address;

  /**
   * Consider MethodNum numbers to be similar in concerns as for offsets in
   * function tables (in programming languages), and for tags in ProtocolBuffer
   * fields. Tags in ProtocolBuffers recommend assigning a unique tag to a field
   * and never reusing that tag. If a field is no longer used, the field name
   * may change but should still remain defined in the code to ensure the tag
   * number is not reused accidentally. The same should apply to the MethodNum
   * associated with methods in Filecoin VM Actors.
   */
  struct MethodNumber : boost::totally_ordered<MethodNumber> {
    constexpr MethodNumber() : method_number{} {}

    constexpr MethodNumber(uint64_t method_number)
        : method_number{method_number} {}

    inline bool operator==(const MethodNumber &other) const {
      return method_number == other.method_number;
    }

    inline bool operator<(const MethodNumber &other) const {
      return method_number < other.method_number;
    }

    uint64_t method_number;
  };

  CBOR_ENCODE(MethodNumber, method) {
    return s << method.method_number;
  }

  CBOR_DECODE(MethodNumber, method) {
    return s >> method.method_number;
  }

  /**
   * MethodParams is serialized parameters to the method call
   */
  class MethodParams : public Buffer {
    using Buffer::Buffer;

   public:
    inline bool operator==(const MethodParams &other) const {
      return Buffer::operator==(static_cast<const Buffer &>(other));
    }
  };

  /**
   * CodeID identifies an actor's code (either one of the builtin actors, or, in
   * the future, potentially a CID of VM code for a custom actor)
   */
  class CodeId : public CID {
   public:
    CodeId() = default;
    explicit CodeId(CID cid) : CID{std::move(cid)} {}
  };

  class ActorSubstateCID : public CID {
   public:
    ActorSubstateCID() = default;
    explicit ActorSubstateCID(CID cid) : CID{std::move(cid)} {}
  };

  /**
   * Common actor state interface represents the on-chain storage all actors
   * keep
   */
  struct Actor {
    /// Identifies the code this actor executes
    CodeId code{};
    /// CID of the root of optional actor-specific sub-state
    ActorSubstateCID head{};
    /// Expected sequence number of the next message sent by this actor
    uint64_t nonce{};
    /// Balance of tokens held by this actor
    BigInt balance{};
  };

  bool operator==(const Actor &lhs, const Actor &rhs);

  CBOR_TUPLE(Actor, code, head, nonce, balance)

  /** Check if code specifies builtin actor implementation */
  bool isBuiltinActor(const CodeId &code);

  /** Check if only one instance of actor should exists */
  bool isSingletonActor(const CodeId &code);

  /** Check if actor code can represent external signing parties */
  bool isSignableActor(const CodeId &code);

  /** Reserved method number for send operation */
  constexpr MethodNumber kSendMethodNumber{0};

  /** Reserved method number for constructor */
  constexpr MethodNumber kConstructorMethodNumber{1};

  extern const CID kEmptyObjectCid;

  extern const CodeId kAccountCodeCid, kCronCodeCid, kStoragePowerCodeCid,
      kStorageMarketCodeCid, kStorageMinerCodeCid, kMultisigCodeCid,
      kInitCodeCid, kPaymentChannelCodeCid;

  inline static const auto kSystemActorAddress = Address::makeFromId(0);
  inline static const auto kInitAddress = Address::makeFromId(0);
  inline static const auto kStoragePowerAddress = Address::makeFromId(2);
  inline static const auto kCronAddress = Address::makeFromId(4);
  inline static const auto kStorageMarketAddress = Address::makeFromId(5);
  inline static const auto kBurntFundsActorAddress = Address::makeFromId(99);
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
