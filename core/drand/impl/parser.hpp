/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_PROTOBUF_MARSHALLER_HPP
#define CPP_FILECOIN_CORE_DRAND_PROTOBUF_MARSHALLER_HPP

#include "drand/messages.hpp"

/// namespace and classes produced by protoc
namespace drand {
  class Identity;
  class Node;
  class GroupPacket;
  class PublicRandResponse;
}  // namespace drand

namespace fc::drand {

  class ProtoParser {
   public:
    static Identity protoToHandy(const ::drand::Identity &identity);

    static Node protoToHandy(const ::drand::Node &node);

    static GroupPacket protoToHandy(const ::drand::GroupPacket &group_packet);

    static PublicRandResponse protoToHandy(
        const ::drand::PublicRandResponse &rand_response);
  };
}  // namespace fc::drand

#endif  // CPP_FILECOIN_CORE_DRAND_PROTOBUF_MARSHALLER_HPP
