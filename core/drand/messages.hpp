/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP
#define CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP

#include <string>
#include <vector>

#include "codec/cbor/streams_annotation.hpp"

namespace fc::drand {

  using Bytes = std::vector<uint8_t>;

  struct BeaconEntry {
    uint64_t round;
    Bytes data;  // which is PublicRandResponse.signature
  };
  CBOR_TUPLE(BeaconEntry, round, data)
  inline bool operator==(const BeaconEntry &lhs, const BeaconEntry &rhs) {
    return lhs.round == rhs.round && lhs.data == rhs.data;
  }

  /**
   * Below are handy analogies of proto-generated message structures that
   * are used in public API of Drand protocol.
   */

  struct Identity {
    std::string address;
    Bytes key;
    bool tls;
  };

  struct GroupPacket {
    std::vector<Identity> nodes;
    uint32_t threshold;
    uint32_t period;  // in seconds
    uint64_t genesis_time;
    uint64_t transition_time;
    Bytes genesis_seed;
    std::vector<Bytes> dist_key;
  };

  struct PublicRandResponse {
    uint64_t round;
    Bytes signature;
    Bytes previous_signature;
    Bytes randomness;
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP
