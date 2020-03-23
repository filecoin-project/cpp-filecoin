/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "request_builder.hpp"

#include "codec/cbor/cbor_encode_stream.hpp"

#include "protobuf/message.pb.h"

namespace fc::storage::ipfs::graphsync {

  using codec::cbor::CborEncodeStream;

  void RequestBuilder::addRequest(RequestId request_id,
                                  const CID &root_cid,
                                  gsl::span<const uint8_t> selector,
                                  const std::vector<Extension> &extensions) {
    using codec::cbor::CborEncodeStream;

    auto *dst = pb_msg_->add_requests();
    dst->set_id(request_id);

    CborEncodeStream encoder;
    encoder << root_cid;
    auto d = encoder.data();

    dst->set_root(d.data(), d.size());
    if (!selector.empty()) {
      dst->set_selector(selector.data(), selector.size());
    }

    dst->set_priority(1);

    for (auto extension : extensions) {
      dst->mutable_extensions()->insert(
          {std::string(extension.name),
           std::string(extension.data.begin(), extension.data.end())});
    }

    empty_ = false;
  }

  void RequestBuilder::addCancelRequest(RequestId request_id) {
    auto *dst = pb_msg_->add_requests();
    dst->set_id(request_id);
    dst->set_cancel(true);
    empty_ = false;
  }

  void RequestBuilder::setCompleteRequestList() {
    pb_msg_->set_completerequestlist(true);
    empty_ = false;
  }

}  // namespace fc::storage::ipfs::graphsync
