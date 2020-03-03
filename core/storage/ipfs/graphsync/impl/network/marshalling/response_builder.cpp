/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "response_builder.hpp"

#include "codec/cbor/cbor_encode_stream.hpp"

#include "protobuf/message.pb.h"

namespace fc::storage::ipfs::graphsync {

  namespace {
    using codec::cbor::CborEncodeStream;

    // encodes metadata item, {"link":CID, "blockPresent":bool}
    std::map<std::string, CborEncodeStream> encodeMetadataItem(
        const std::pair<CID, bool> &item) {
      static const std::string link(kLink);
      static const std::string blockPresent(kBlockPresent);

      std::map<std::string, CborEncodeStream> m;
      m[link] << item.first;
      m[blockPresent] << item.second;
      return m;
    }

    // encodes metadata pairs, [ {"link":CID, "blockPresent":bool}, ...]
    std::string encodeMetadata(const ResponseMetadata &metadata) {
      auto l = CborEncodeStream::list();

      for (const auto &item : metadata) {
        l << encodeMetadataItem(item);
      }

      CborEncodeStream encoder;
      encoder << l;
      auto d = encoder.data();
      std::string s(d.begin(), d.end());
      return s;
    }
  }  // namespace

  void ResponseBuilder::addResponse(int request_id,
                                    ResponseStatusCode status,
                                    const ResponseMetadata &metadata) {
    auto *dst = pb_msg_->add_responses();
    dst->set_id(request_id);
    dst->set_status(status);

    if (!metadata.empty()) {
      dst->mutable_extensions()->insert(
          {std::string(kResponseMetadata), encodeMetadata(metadata)});
    }

    empty_ = false;
  }

  void ResponseBuilder::addDataBlock(
      const CID &cid, const common::Buffer &data) {
    auto *dst = pb_msg_->add_data();

    CborEncodeStream encoder;
    encoder << cid;
    auto d = encoder.data();
    dst->set_prefix(d.data(), d.size());
    dst->set_data(data.data(), data.size());
    empty_ = false;
  }

}  // namespace fc::storage::ipfs::graphsync
