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

    for (auto extension : extensions) {
      dst->mutable_extensions()->insert(
          {std::string(extension.name),
           std::string(extension.data.begin(), extension.data.end())});
    }

    empty_ = false;
  }

  void ResponseBuilder::addDataBlock(const CID &cid,
                                     const common::Buffer &data) {
    auto *dst = pb_msg_->add_data();

    CborEncodeStream encoder;
    encoder << cid;
    auto d = encoder.data();
    dst->set_prefix(d.data(), d.size());
    dst->set_data(data.data(), data.size());
    empty_ = false;
  }

}  // namespace fc::storage::ipfs::graphsync
