/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include <string>
#include <vector>
#include "crypto/hasher/hasher.hpp"

namespace fc::vm::actor {
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
}  // namespace fc::vm::actor
