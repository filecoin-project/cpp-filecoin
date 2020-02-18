/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_impl.hpp"

#include <cassert>

#include "network/server.hpp"
#include "marshalling/request_builder.hpp"

namespace fc::storage::ipfs::graphsync {

  Subscription GraphsyncImpl::start(std::shared_ptr<MerkleDagBridge> dag,
                                    Graphsync::BlockCallback callback) {
    assert(dag);
    assert(callback);

    server_->start();
    started_ = true;
    return block_subscription_->start(std::move(callback));
  }

  Subscription GraphsyncImpl::makeRequest(
      libp2p::peer::PeerId peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      gsl::span<const uint8_t> root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      std::unordered_set<common::Buffer> dont_send_cids) {

  }



}  // namespace fc::storage::ipfs::graphsync
