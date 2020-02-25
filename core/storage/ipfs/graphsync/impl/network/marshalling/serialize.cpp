/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <google/protobuf/message_lite.h>

#include <libp2p/multi/uvarint.hpp>

#include "serialize.hpp"

namespace fc::storage::ipfs::graphsync {

  boost::optional<std::shared_ptr<const libp2p::common::ByteArray>>
  serializeProtobufMessage(const google::protobuf::MessageLite &msg) {
    size_t msg_sz = msg.ByteSizeLong();

    auto varint_len = libp2p::multi::UVarint{msg_sz};
    const auto &varint_vec = varint_len.toVector();
    size_t prefix_sz = varint_vec.size();

    auto buffer =
        std::make_shared<const libp2p::common::ByteArray>(prefix_sz + msg_sz);

    // NOLINTNEXTLINE
    uint8_t *data = const_cast<uint8_t *>(buffer->data());

    memcpy(data, varint_vec.data(), prefix_sz);

    // NOLINTNEXTLINE
    if (!msg.SerializeToArray(data + prefix_sz, msg_sz)) {
      return boost::none;
    }
    return buffer;
  }

}  // namespace fc::storage::ipfs::graphsync
