/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "actor/actor.hpp"

#include <string>
#include <vector>

#include <libp2p/crypto/sha/sha256.hpp>

namespace fc::actor {

  ContentIdentifier kEmptyObjectCid{
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

  ContentIdentifier kAccountCodeCid = makeRawIdentityCid("fil/1/account");
  ContentIdentifier kCronCodeCid = makeRawIdentityCid("fil/1/cron");
  ContentIdentifier kStoragePowerCodeCid = makeRawIdentityCid("fil/1/power");
  ContentIdentifier kStorageMarketCodeCid = makeRawIdentityCid("fil/1/market");
  ContentIdentifier kStorageMinerCodeCid = makeRawIdentityCid("fil/1/miner");
  ContentIdentifier kMultisigCodeCid = makeRawIdentityCid("fil/1/multisig");
  ContentIdentifier kInitCodeCid = makeRawIdentityCid("fil/1/init");
  ContentIdentifier kPaymentChannelCodeCid = makeRawIdentityCid("fil/1/paych");
}  // namespace fc::actor
