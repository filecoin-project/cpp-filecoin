/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/operators.hpp>

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "primitives/address/address.hpp"
#include "primitives/big_int.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor {

  using fc::common::Buffer;
  using primitives::BigInt;
  using primitives::Nonce;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using version::NetworkVersion;

  /**
   * Actor version
   */
  enum class ActorVersion {
    kVersion0 = 0,
    kVersion2 = 2,
    kVersion3 = 3,
    kVersion4 = 4,
    kVersion5 = 5,
  };

  /**
   * Consider MethodNum numbers to be similar in concerns as for offsets in
   * function tables (in programming languages), and for tags in ProtocolBuffer
   * fields. Tags in ProtocolBuffers recommend assigning a unique tag to a field
   * and never reusing that tag. If a field is no longer used, the field name
   * may change but should still remain defined in the code to ensure the tag
   * number is not reused accidentally. The same should apply to the MethodNum
   * associated with methods in Filecoin VM Actors.
   */
  using MethodNumber = uint64_t;

  using MethodParams = Buffer;

  using CodeId = CID;

  /**
   * Common actor state interface represents the on-chain storage all actors
   * keep
   */
  struct Actor {
    /// Identifies the code this actor executes
    CodeId code{};
    /// CID of the root of optional actor-specific sub-state
    CID head{};
    /// Expected sequence number of the next message sent by this actor
    Nonce nonce{};
    /// Balance of tokens held by this actor
    TokenAmount balance{};
  };
  CBOR_TUPLE(Actor, code, head, nonce, balance)

  inline bool operator==(const Actor &lhs, const Actor &rhs) {
    return lhs.code == rhs.code && lhs.head == rhs.head
           && lhs.nonce == rhs.nonce && lhs.balance == rhs.balance;
  }

  /** Reserved method number for send operation */
  constexpr MethodNumber kSendMethodNumber{0};

  /** Reserved method number for constructor */
  constexpr MethodNumber kConstructorMethodNumber{1};

  inline static const CID kEmptyObjectCid{CbCid::hash(BytesN<1>{0x80})};

  inline static const auto kSystemActorAddress = Address::makeFromId(0);
  inline static const auto kInitAddress = Address::makeFromId(1);
  inline static const auto kRewardAddress = Address::makeFromId(2);
  inline static const auto kCronAddress = Address::makeFromId(3);
  inline static const auto kStoragePowerAddress = Address::makeFromId(4);
  inline static const auto kStorageMarketAddress = Address::makeFromId(5);
  inline static const auto kVerifiedRegistryAddress = Address::makeFromId(6);
  inline static const auto kReserveActorAddress = Address::makeFromId(90);
  inline static const auto kBurntFundsActorAddress = Address::makeFromId(99);
}  // namespace fc::vm::actor
