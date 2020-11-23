/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "response_builder.hpp"

#include "codec/cbor/cbor.hpp"

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

    auto insertExtension = [&](std::string name, BytesIn data) {
      dst->mutable_extensions()->insert(
          {std::move(name), std::string(data.begin(), data.end())});
    };

    insertExtension(std::string{kResponseMetadataProtocol},
                    codec::cbor::encode(meta_).value());

    for (auto extension : extensions) {
      insertExtension(extension.name, extension.data);
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
