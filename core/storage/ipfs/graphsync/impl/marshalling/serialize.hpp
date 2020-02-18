/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_SERIALIZE_HPP
#define CPP_FILECOIN_GRAPHSYNC_SERIALIZE_HPP

// TODO(artem) move to libp2p/protocol/common

#include <memory>

#include <boost/optional.hpp>

#include <libp2p/common/byteutil.hpp>

namespace google::protobuf {
  class MessageLite;
}

namespace fc::storage::ipfs::graphsync {

  /**
   * Tries to serialize generic protobuf message into shared array of bytes
   * with varint length prefix
   * @param msg protobuf message
   * @return shared buffer to serialized bytes, none if serialize failed, i.e.
   * not all required fields are set in the msg
   */
  boost::optional<std::shared_ptr<const libp2p::common::ByteArray>>
  serializeProtobufMessage(const google::protobuf::MessageLite &msg);

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_SERIALIZE_HPP
