/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP
#define CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "crypto/bls/bls_types.hpp"

namespace fc::drand {
  using std::chrono::seconds;
  using Round = uint64_t;

  struct BeaconEntry {
    Round round;
    Buffer data;  // BlsSignature, inconsistent size in lotus
  };
  CBOR_TUPLE(BeaconEntry, round, data)
  inline bool operator==(const BeaconEntry &lhs, const BeaconEntry &rhs) {
    return lhs.round == rhs.round && lhs.data == rhs.data;
  }

  struct ChainInfo {
    BlsPublicKey key;
    seconds genesis, period;
  };

  struct PublicRandResponse {
    Round round;
    BlsSignature signature;
    Buffer prev;  // Hash256 if genesis else BlsSignature
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP
