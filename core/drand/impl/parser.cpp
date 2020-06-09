/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parser.hpp"

#include "drand/protobuf/api.pb.h"

namespace fc::drand {

  Identity ProtoParser::protoToHandy(const ::drand::Identity &identity) {
    return Identity{.address = identity.address(),
                    .key = {identity.key().begin(), identity.key().end()},
                    .tls = identity.tls()};
  }
  Node ProtoParser::protoToHandy(const ::drand::Node &node) {
    return Node{.public_identity = protoToHandy(node.public_()),
                .index = node.index()};
  }
  GroupPacket ProtoParser::protoToHandy(
      const ::drand::GroupPacket &group_packet) {
    std::vector<Node> nodes;
    nodes.reserve(group_packet.nodes_size());
    for (auto i = 0; i < group_packet.nodes_size(); ++i) {
      nodes.push_back(protoToHandy(group_packet.nodes(i)));
    }
    std::vector<Bytes> dist_key;
    dist_key.reserve(group_packet.dist_key_size());
    for (int i = 0; i < group_packet.dist_key_size(); ++i) {
      dist_key.emplace_back(group_packet.dist_key(i).begin(),
                            group_packet.dist_key(i).end());
    }
    return GroupPacket{.nodes = std::move(nodes),
                       .threshold = group_packet.threshold(),
                       .period = group_packet.period(),
                       .genesis_time = group_packet.genesis_time(),
                       .transition_time = group_packet.transition_time(),
                       .genesis_seed = {group_packet.genesis_seed().begin(),
                                        group_packet.genesis_seed().end()},
                       .dist_key = std::move(dist_key)};
  }

  PublicRandResponse ProtoParser::protoToHandy(
      const ::drand::PublicRandResponse &rand_response) {
    return PublicRandResponse{
        .round = rand_response.round(),
        .signature = {rand_response.signature().begin(),
                      rand_response.signature().end()},
        .previous_signature = {rand_response.previous_signature().begin(),
                               rand_response.previous_signature().end()},
        .randomness = {rand_response.randomness().begin(),
                       rand_response.randomness().end()}};
  }

}  // namespace fc::drand
