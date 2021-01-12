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
#include "vm/version.hpp"

namespace fc::vm::actor {

  using fc::common::Buffer;
  using primitives::BigInt;
  using primitives::address::Address;
  using version::NetworkVersion;

  /**
   * Actor version v0 or v2
   */
  enum class ActorVersion {
    kVersion0 = 0,
    kVersion2 = 2,
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
    uint64_t nonce{};
    /// Balance of tokens held by this actor
    BigInt balance{};
  };

  bool operator==(const Actor &lhs, const Actor &rhs);

  CBOR_TUPLE(Actor, code, head, nonce, balance)

  /** Checks if code is an account actor */
  bool isAccountActor(const CodeId &code);

  /** Checks if code is miner actor */
  bool isStorageMinerActor(const CodeId &code);

  /** Check if code specifies builtin actor implementation */
  bool isBuiltinActor(const CodeId &code);

  /** Check if only one instance of actor should exists */
  bool isSingletonActor(const CodeId &code);

  /** Check if actor code can represent external signing parties */
  bool isSignableActor(const CodeId &code);

  /** Make code cid from raw string */
  CID makeRawIdentityCid(const std::string &str);

  /**
   * Returns actor version for network version
   *
   * Network version [0..3] => Actor version v0
   * Network version [4..?] => Actor version v2
   *
   * @param network_version - version of network
   * @return v0 or v2 actor version
   */
  ActorVersion getActorVersionForNetwork(const NetworkVersion &network_version);

  /** Reserved method number for send operation */
  constexpr MethodNumber kSendMethodNumber{0};

  /** Reserved method number for constructor */
  constexpr MethodNumber kConstructorMethodNumber{1};

  extern const CID kEmptyObjectCid;

  inline static const auto kSystemActorAddress = Address::makeFromId(0);
  inline static const auto kInitAddress = Address::makeFromId(1);
  inline static const auto kRewardAddress = Address::makeFromId(2);
  inline static const auto kCronAddress = Address::makeFromId(3);
  inline static const auto kStoragePowerAddress = Address::makeFromId(4);
  inline static const auto kStorageMarketAddress = Address::makeFromId(5);
  inline static const auto kVerifiedRegistryAddress = Address::makeFromId(6);
  inline static const auto kBurntFundsActorAddress = Address::makeFromId(99);
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_HPP
