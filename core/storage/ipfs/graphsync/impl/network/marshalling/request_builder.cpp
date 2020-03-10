/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "request_builder.hpp"

#include "codec/cbor/cbor_encode_stream.hpp"

#include "protobuf/message.pb.h"

namespace fc::storage::ipfs::graphsync {

  using codec::cbor::CborEncodeStream;

  namespace {
    // returs CBOR boolean true as string
    const std::string& encodeTrue() {
      static const std::string s("\xF5");
      return s;
    }

    // encodes list of CIDs into CBOR
    std::string encodeCids(const std::vector<CID> &dont_send_cids) {
      CborEncodeStream encoder;
      encoder << dont_send_cids;
      auto d = encoder.data();
      std::string s(d.begin(), d.end());
      return s;
    }
  }  // namespace

  void RequestBuilder::addRequest(RequestId request_id,
                                  const CID &root_cid,
                                  gsl::span<const uint8_t> selector,
                                  bool need_metadata,
                                  const std::vector<CID> &dont_send_cids) {
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

    if (need_metadata) {
      dst->mutable_extensions()->insert(
          {std::string(kResponseMetadata), encodeTrue()});
    }

    if (!dont_send_cids.empty()) {
      dst->mutable_extensions()->insert(
          {std::string(kDontSendCids), encodeCids(dont_send_cids)});
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
