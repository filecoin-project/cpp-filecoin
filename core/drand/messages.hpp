/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP
#define CPP_FILECOIN_CORE_DRAND_PROTOBUF_MESSAGES_HPP

#include <string>
#include <vector>

/**
 * The file contains handy analogies of proto-generated message structures that
 * are used in public API of Drand protocol.
 */

namespace fc::drand {

  using Bytes = std::vector<uint8_t>;

  struct Identity {
    std::string address;
    Bytes key;
    bool tls;
  };

  struct Node {
    Identity public_identity;  // named as "public" in drand proto
    uint32_t index;
  };

  struct GroupPacket {
    std::vector<Node> nodes;
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
