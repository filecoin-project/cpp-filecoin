/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include <string>
#include <vector>

#include <libp2p/crypto/sha/sha256.hpp>

namespace fc::vm::actor {
  bool isBuiltinActor(const ContentIdentifier &code) {
    return code == kStorageMarketCodeCid || code == kStoragePowerCodeCid
           || code == kStorageMinerCodeCid || code == kAccountCodeCid
           || code == kInitCodeCid || code == kMultisigCodeCid
           || code == kPaymentChannelCodeCid;
  }

  bool isSingletonActor(const ContentIdentifier &code) {
    return code == kStoragePowerCodeCid || code == kStorageMarketCodeCid
           || code == kInitCodeCid || code == kCronCodeCid;
  }

  const ContentIdentifier kEmptyObjectCid{
      ContentIdentifier::Version::V1,
      libp2p::multi::MulticodecType::Code::DAG_CBOR,
      libp2p::multi::Multihash::create(libp2p::multi::HashType::sha256,
                                       libp2p::crypto::sha256({"\xA0", 1}))
          .value()};

  ContentIdentifier makeRawIdentityCid(const std::string &str) {
    std::vector<uint8_t> bytes{str.begin(), str.end()};
    return {ContentIdentifier::Version::V1,
            libp2p::multi::MulticodecType::Code::RAW,
            libp2p::multi::Multihash::create(libp2p::multi::HashType::identity,
                                             bytes)
                .value()};
  }

  const ContentIdentifier kAccountCodeCid = makeRawIdentityCid("fil/1/account");
  const ContentIdentifier kCronCodeCid = makeRawIdentityCid("fil/1/cron");
  const ContentIdentifier kStoragePowerCodeCid = makeRawIdentityCid("fil/1/power");
  const ContentIdentifier kStorageMarketCodeCid = makeRawIdentityCid("fil/1/market");
  const ContentIdentifier kStorageMinerCodeCid = makeRawIdentityCid("fil/1/miner");
  const ContentIdentifier kMultisigCodeCid = makeRawIdentityCid("fil/1/multisig");
  const ContentIdentifier kInitCodeCid = makeRawIdentityCid("fil/1/init");
  const ContentIdentifier kPaymentChannelCodeCid = makeRawIdentityCid("fil/1/paych");
}  // namespace fc::vm::actor
