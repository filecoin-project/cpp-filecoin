/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include <string>
#include <vector>

#include <libp2p/crypto/sha/sha256.hpp>

namespace fc::vm::actor {
  bool operator==(const Actor &lhs, const Actor &rhs) {
    return lhs.code == rhs.code && lhs.head == rhs.head
           && lhs.nonce == rhs.nonce && lhs.balance == rhs.balance;
  }

  bool isBuiltinActor(const CodeId &code) {
    return code == kStorageMarketCodeCid || code == kStoragePowerCodeCid
           || code == kStorageMinerCodeCid || code == kAccountCodeCid
           || code == kInitCodeCid || code == kMultisigCodeCid
           || code == kPaymentChannelCodeCid;
  }

  bool isSingletonActor(const CodeId &code) {
    return code == kStoragePowerCodeCid || code == kStorageMarketCodeCid
           || code == kInitCodeCid || code == kCronCodeCid;
  }

  const CID kEmptyObjectCid{
      CID::Version::V1,
      libp2p::multi::MulticodecType::Code::DAG_CBOR,
      libp2p::multi::Multihash::create(libp2p::multi::HashType::sha256,
                                       libp2p::crypto::sha256({"\xA0", 1}))
          .value()};

  CID makeRawIdentityCid(const std::string &str) {
    std::vector<uint8_t> bytes{str.begin(), str.end()};
    return {CID::Version::V1,
            libp2p::multi::MulticodecType::Code::RAW,
            libp2p::multi::Multihash::create(libp2p::multi::HashType::identity,
                                             bytes)
                .value()};
  }

  const CodeId kAccountCodeCid = CodeId(makeRawIdentityCid("fil/1/account"));
  const CodeId kCronCodeCid = CodeId(makeRawIdentityCid("fil/1/cron"));
  const CodeId kStoragePowerCodeCid = CodeId(makeRawIdentityCid("fil/1/power"));
  const CodeId kStorageMarketCodeCid =
      CodeId(makeRawIdentityCid("fil/1/market"));
  const CodeId kStorageMinerCodeCid = CodeId(makeRawIdentityCid("fil/1/miner"));
  const CodeId kMultisigCodeCid = CodeId(makeRawIdentityCid("fil/1/multisig"));
  const CodeId kInitCodeCid = CodeId(makeRawIdentityCid("fil/1/init"));
  const CodeId kPaymentChannelCodeCid =
      CodeId(makeRawIdentityCid("fil/1/paych"));

}  // namespace fc::vm::actor
