/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include <string>
#include <vector>
#include "crypto/hasher/hasher.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::actor {

  using namespace builtin;

  bool operator==(const Actor &lhs, const Actor &rhs) {
    return lhs.code == rhs.code && lhs.head == rhs.head
           && lhs.nonce == rhs.nonce && lhs.balance == rhs.balance;
  }

  bool isAccountActor(const CodeId &code) {
    return code == v0::kAccountCodeId || code == v2::kAccountCodeId
           || code == v3::kAccountCodeId;
  }

  bool isStorageMinerActor(const CodeId &code) {
    return code == v0::kStorageMinerCodeId || code == v2::kStorageMinerCodeId;
  }

  bool isBuiltinActor(const CodeId &code) {
    return code == v0::kStorageMarketCodeId || code == v0::kStoragePowerCodeId
           || code == v0::kStorageMinerCodeId || code == v0::kAccountCodeId
           || code == v0::kInitCodeId || code == v0::kMultisigCodeId
           || code == v0::kPaymentChannelCodeCid || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId
           || code == v2::kStorageMarketCodeId
           || code == v2::kStoragePowerCodeId
           || code == v2::kStorageMinerCodeId || code == v2::kAccountCodeId
           || code == v2::kInitCodeId || code == v2::kMultisigCodeId
           || code == v2::kPaymentChannelCodeCid || code == v2::kCronCodeId
           || code == v2::kRewardActorCodeId || code == v2::kSystemActorCodeId
           || code == v3::kStorageMarketCodeId
           || code == v3::kStoragePowerCodeId
           || code == v3::kStorageMinerCodeId || code == v3::kAccountCodeId
           || code == v3::kInitCodeId || code == v3::kMultisigCodeId
           || code == v3::kPaymentChannelCodeCid || code == v3::kCronCodeId
           || code == v3::kRewardActorCodeId || code == v3::kSystemActorCodeId;
  }

  bool isSingletonActor(const CodeId &code) {
    return code == v0::kStoragePowerCodeId || code == v0::kStorageMarketCodeId
           || code == v0::kInitCodeId || code == v0::kCronCodeId
           || code == v0::kRewardActorCodeId || code == v0::kSystemActorCodeId
           || code == v2::kStoragePowerCodeId
           || code == v2::kStorageMarketCodeId || code == v2::kInitCodeId
           || code == v2::kCronCodeId || code == v2::kRewardActorCodeId
           || code == v2::kSystemActorCodeId || code == v3::kStoragePowerCodeId
           || code == v3::kStorageMarketCodeId || code == v3::kInitCodeId
           || code == v3::kCronCodeId || code == v3::kRewardActorCodeId
           || code == v3::kSystemActorCodeId;
  }

  bool isSignableActor(const CodeId &code) {
    return code == v0::kAccountCodeId || code == v0::kMultisigCodeId
           || code == v2::kAccountCodeId || code == v2::kMultisigCodeId
           || code == v3::kAccountCodeId || code == v3::kMultisigCodeId;
  }

  static uint8_t kCborEmptyList[]{0x80};
  const CID kEmptyObjectCid{CID::Version::V1,
                            libp2p::multi::MulticodecType::Code::DAG_CBOR,
                            crypto::Hasher::blake2b_256(kCborEmptyList)};

  CID makeRawIdentityCid(const std::string &str) {
    std::vector<uint8_t> bytes{str.begin(), str.end()};
    return {CID::Version::V1,
            libp2p::multi::MulticodecType::Code::RAW,
            libp2p::multi::Multihash::create(libp2p::multi::HashType::identity,
                                             bytes)
                .value()};
  }

  ActorVersion getActorVersionForNetwork(
      const NetworkVersion &network_version) {
    switch (network_version) {
      case NetworkVersion::kVersion0:
      case NetworkVersion::kVersion1:
      case NetworkVersion::kVersion2:
      case NetworkVersion::kVersion3:
        return ActorVersion::kVersion0;
      case NetworkVersion::kVersion4:
      case NetworkVersion::kVersion5:
      case NetworkVersion::kVersion6:
      case NetworkVersion::kVersion7:
      case NetworkVersion::kVersion8:
      case NetworkVersion::kVersion9:
        return ActorVersion::kVersion2;
      case NetworkVersion::kVersion10:
      case NetworkVersion::kVersion11:
      case NetworkVersion::kVersion12:
        return ActorVersion::kVersion3;
    }
  }

  ActorVersion getActorVersionForCid(const CodeId &actorCid) {
    if ((actorCid == v0::kAccountCodeId) || (actorCid == v0::kCronCodeId)
        || (actorCid == v0::kStoragePowerCodeId)
        || (actorCid == v0::kStorageMarketCodeId)
        || (actorCid == v0::kStorageMinerCodeId)
        || (actorCid == v0::kMultisigCodeId) || (actorCid == v0::kInitCodeId)
        || (actorCid == v0::kPaymentChannelCodeCid)
        || (actorCid == v0::kRewardActorCodeId)
        || (actorCid == v0::kSystemActorCodeId)
        || (actorCid == v0::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion0;
    }

    if ((actorCid == v2::kAccountCodeId) || (actorCid == v2::kCronCodeId)
        || (actorCid == v2::kStoragePowerCodeId)
        || (actorCid == v2::kStorageMarketCodeId)
        || (actorCid == v2::kStorageMinerCodeId)
        || (actorCid == v2::kMultisigCodeId) || (actorCid == v2::kInitCodeId)
        || (actorCid == v2::kPaymentChannelCodeCid)
        || (actorCid == v2::kRewardActorCodeId)
        || (actorCid == v2::kSystemActorCodeId)
        || (actorCid == v2::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion2;
    }

    if ((actorCid == v3::kAccountCodeId) || (actorCid == v3::kCronCodeId)
        || (actorCid == v3::kStoragePowerCodeId)
        || (actorCid == v3::kStorageMarketCodeId)
        || (actorCid == v3::kStorageMinerCodeId)
        || (actorCid == v3::kMultisigCodeId) || (actorCid == v3::kInitCodeId)
        || (actorCid == v3::kPaymentChannelCodeCid)
        || (actorCid == v3::kRewardActorCodeId)
        || (actorCid == v3::kSystemActorCodeId)
        || (actorCid == v3::kVerifiedRegistryCodeId)) {
      return ActorVersion::kVersion3;
    }

    assert(false);
  }

}  // namespace fc::vm::actor
