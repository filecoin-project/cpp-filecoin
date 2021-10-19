/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

#include "codec/cbor/streams_annotation.hpp"
#include "common/bytes.hpp"
#include "crypto/bls/bls_types.hpp"

namespace fc::drand {
  using std::chrono::seconds;
  using Round = uint64_t;

  struct BeaconEntry {
    Round round = 0;
    Bytes data;  // BlsSignature, inconsistent size in lotus
  };
  CBOR_TUPLE(BeaconEntry, round, data)
  inline bool operator==(const BeaconEntry &lhs, const BeaconEntry &rhs) {
    return lhs.round == rhs.round && lhs.data == rhs.data;
  }

  struct ChainInfo {
    BlsPublicKey key;
    seconds genesis{};
    seconds period{};
  };

  struct PublicRandResponse {
    Round round;
    BlsSignature signature;
    Bytes prev;  // Hash256 if genesis else BlsSignature
  };
}  // namespace fc::drand
