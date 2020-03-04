/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_impl.hpp"

#include <cassert>

#include "marshalling/request_builder.hpp"
#include "network/server.hpp"
#include "local_requests.hpp"

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
      const libp2p::peer::PeerId& peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      gsl::span<const uint8_t> root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      const std::vector<CID> &dont_send_cids,
      RequestProgressCallback callback) {
    if (!started_) {
      // TODO defer RS_GRAPHSYNC_STOPPED to callback
      return Subscription();
    }
    return local_requests_->makeRequest(std::move(peer),
                                        std::move(address),
                                        root_cid,
                                        selector,
                                        need_metadata,
                                        dont_send_cids,
                                        std::move(callback));
  }

}  // namespace fc::storage::ipfs::graphsync
