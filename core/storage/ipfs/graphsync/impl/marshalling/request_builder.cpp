/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "request_builder.hpp"

#include <protobuf/message.pb.h>

namespace fc::storage::ipfs::graphsync {

  void RequestBuilder::addRequest(const Message::Request& req) {
    auto *dst = pb_msg_->add_requests();
    dst->set_id(req.id);
    if (req.cancel) {
      dst->set_cancel(true);
    } else {
      dst->set_root(req.root_cid.data(), req.root_cid.size());
      dst->set_selector(req.selector);
      dst->set_priority(req.priority);

      // TODO(FIL-96): encode metadata: do_not_send and send_metadata
    }
    empty_ = false;
  }

  void RequestBuilder::setCompleteRequestList() {
    pb_msg_->set_completerequestlist(true);
    empty_ = false;
  }

}  // namespace fc::storage::ipfs::graphsync
