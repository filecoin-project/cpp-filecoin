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

}  // namespace fc::vm::actor
