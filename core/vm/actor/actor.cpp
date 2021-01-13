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
    return code == v0::kAccountCodeCid || code == v2::kAccountCodeCid
           || code == v3::kAccountCodeCid;
  }

  bool isStorageMinerActor(const CodeId &code) {
    return code == v0::kStorageMinerCodeCid || code == v2::kStorageMinerCodeCid;
  }

  bool isBuiltinActor(const CodeId &code) {
    return code == v0::kStorageMarketCodeCid || code == v0::kStoragePowerCodeCid
           || code == v0::kStorageMinerCodeCid || code == v0::kAccountCodeCid
           || code == v0::kInitCodeCid || code == v0::kMultisigCodeCid
           || code == v0::kPaymentChannelCodeCid || code == v0::kCronCodeCid
           || code == v0::kRewardActorCodeID || code == v0::kSystemActorCodeID
           || code == v2::kStorageMarketCodeCid
           || code == v2::kStoragePowerCodeCid
           || code == v2::kStorageMinerCodeCid || code == v2::kAccountCodeCid
           || code == v2::kInitCodeCid || code == v2::kMultisigCodeCid
           || code == v2::kPaymentChannelCodeCid || code == v2::kCronCodeCid
           || code == v2::kRewardActorCodeID || code == v2::kSystemActorCodeID
           || code == v3::kStorageMarketCodeCid
           || code == v3::kStoragePowerCodeCid
           || code == v3::kStorageMinerCodeCid || code == v3::kAccountCodeCid
           || code == v3::kInitCodeCid || code == v3::kMultisigCodeCid
           || code == v3::kPaymentChannelCodeCid || code == v3::kCronCodeCid
           || code == v3::kRewardActorCodeID || code == v3::kSystemActorCodeID;
  }

  bool isSingletonActor(const CodeId &code) {
    return code == v0::kStoragePowerCodeCid || code == v0::kStorageMarketCodeCid
           || code == v0::kInitCodeCid || code == v0::kCronCodeCid
           || code == v0::kRewardActorCodeID || code == v0::kSystemActorCodeID
           || code == v2::kStoragePowerCodeCid
           || code == v2::kStorageMarketCodeCid || code == v2::kInitCodeCid
           || code == v2::kCronCodeCid || code == v2::kRewardActorCodeID
           || code == v2::kSystemActorCodeID || code == v3::kStoragePowerCodeCid
           || code == v3::kStorageMarketCodeCid || code == v3::kInitCodeCid
           || code == v3::kCronCodeCid || code == v3::kRewardActorCodeID
           || code == v3::kSystemActorCodeID;
  }

  bool isSignableActorV0(const CodeId &code) {
    return code == v0::kAccountCodeCid || code == v0::kMultisigCodeCid;
  }

  bool isSignableActor(const CodeId &code) {
    return code == v0::kAccountCodeCid || code == v0::kMultisigCodeCid
           || code == v2::kAccountCodeCid || code == v2::kMultisigCodeCid
           || code == v3::kAccountCodeCid || code == v3::kMultisigCodeCid;
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
        return ActorVersion::kVersion2;
    }
  }

}  // namespace fc::vm::actor
