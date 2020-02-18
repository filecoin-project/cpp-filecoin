/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "response_builder.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/common/types.hpp>

#include <protobuf/message.pb.h>

namespace fc::storage::ipfs::graphsync {

  namespace {
    std::vector<uint8_t> encodeCidPrefix(const libp2p::multi::ContentIdentifier &cid) {
      std::vector<uint8_t> bytes;
      bytes.reserve(12);
      if (cid.version == libp2p::multi::ContentIdentifier::Version::V1) {
        bytes.push_back(1);
        libp2p::multi::UVarint type(cid.content_type);
        libp2p::common::append(bytes, type.toBytes());
        libp2p::multi::UVarint hash_type(cid.content_address.getType());
        libp2p::common::append(bytes, hash_type.toBytes());
      } else if (cid.version == libp2p::multi::ContentIdentifier::Version::V0) {
        bytes.push_back(0x12); // sha-256
        bytes.push_back(0x20); // 32 bytes long
      }
      return bytes;
    }
  }

  void ResponseBuilder::addResponse(const Message::Response &resp) {
    auto *dst = pb_msg_->add_responses();
    dst->set_id(resp.id);
    dst->set_status(resp.status);

    // TODO(FIL-96): encode metadata:
    empty_ = false;
  }

  void ResponseBuilder::addDataBlock(
      const libp2p::multi::ContentIdentifier &cid, const ByteArray &data) {
    auto *dst = pb_msg_->add_data();

    auto prefix = encodeCidPrefix(cid);
    dst->set_prefix(prefix.data(), prefix.size());
    dst->set_data(data.data(), data.size());
    empty_ = false;
  }

}  // namespace fc::storage::ipfs::graphsync
