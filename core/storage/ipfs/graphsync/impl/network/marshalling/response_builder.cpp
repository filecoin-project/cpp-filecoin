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
  }  // namespace

  void ResponseBuilder::addResponse(RequestId request_id,
                                    ResponseStatusCode status,
                                    const std::vector<Extension> &extensions) {
    auto *dst = pb_msg_->add_responses();
    dst->set_id(request_id);
    dst->set_status(status);

    auto insertExtension = [](pb::Message_Response* dst, const Extension &e) {
      dst->mutable_extensions()->insert(
          {std::string(e.name),
           std::string(e.data.begin(), e.data.end())});
    };

    if (!meta_.empty()) {
      insertExtension(dst, encodeResponseMetadata(meta_));
    }

    for (auto extension : extensions) {
      insertExtension(dst, extension);
    }

    empty_ = false;
  }

  void ResponseBuilder::addDataBlock(const CID &cid,
                                     const common::Buffer &data) {
    if (data.empty()) {
      meta_.push_back({cid, false});
    } else {
      auto *dst = pb_msg_->add_data();
      OUTCOME_EXCEPT(prefix, cid.getPrefix());
      dst->set_prefix(prefix.data(), prefix.size());
      dst->set_data(data.data(), data.size());
      meta_.push_back({cid, true});
    }

    empty_ = false;
  }

  void ResponseBuilder::clear() {
    meta_.clear();
    MessageBuilder::clear();
  }

}  // namespace fc::storage::ipfs::graphsync
