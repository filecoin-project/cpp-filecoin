/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_impl.hpp"

#include <cassert>

#include "local_requests.hpp"
#include "network/network.hpp"

namespace fc::storage::ipfs::graphsync {

  GraphsyncImpl::GraphsyncImpl(
      std::shared_ptr<libp2p::Host> host,
      std::shared_ptr<libp2p::protocol::Scheduler> scheduler)
      : scheduler_(scheduler),
        network_(std::make_shared<Network>(std::move(host), scheduler)),
        local_requests_(std::make_shared<LocalRequests>(
            std::move(scheduler), [this](RequestId request_id, SharedData body) {
              cancelLocalRequest(request_id, std::move(body));
            })) {}

  GraphsyncImpl::~GraphsyncImpl() {
    doStop();
  }

  void GraphsyncImpl::start(std::shared_ptr<MerkleDagBridge> dag,
                            Graphsync::BlockCallback callback) {
    assert(dag);
    assert(callback);

    network_->start(shared_from_this());
    dag_ = std::move(dag);
    block_cb_ = std::move(callback);
    started_ = true;
  }

  void GraphsyncImpl::stop() {
    doStop();
  }

  void GraphsyncImpl::doStop() {
    if (started_) {
      started_ = false;
      block_cb_ = Graphsync::BlockCallback{};
      dag_.reset();
      network_->stop();
      local_requests_->cancelAll();
    }
  }

  Subscription GraphsyncImpl::makeRequest(
      const libp2p::peer::PeerId &peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      const CID &root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      const std::vector<CID> &dont_send_cids,
      RequestProgressCallback callback) {
    if (!started_ || !network_->canSendRequest(peer)) {
      logger()->trace("makeRequest: rejecting request to peer {}",
          peer.toBase58().substr(46));
      return local_requests_->newRejectedRequest(std::move(callback));
    }

    auto newRequest = local_requests_->newRequest(
        root_cid, selector, need_metadata, dont_send_cids, std::move(callback));

    if (newRequest.request_id > 0) {
      assert(newRequest.body);
      assert(!newRequest.body->empty());

      logger()->trace("makeRequest: sending request to peer {}",
                      peer.toBase58().substr(46));

      network_->makeRequest(peer,
                            std::move(address),
                            newRequest.request_id,
                            std::move(newRequest.body));
    }

    return std::move(newRequest.subscription);
  }

  void GraphsyncImpl::onResponse(const PeerId &peer,
                                 int request_id,
                                 ResponseStatusCode status,
                                 ResponseMetadata metadata) {
    // TODO peer ratings according to status

    if (!started_) {
      return;
    }

    local_requests_->onResponse(request_id, status, std::move(metadata));
  }

  void GraphsyncImpl::onBlock(const PeerId &from,
                              CID cid,
                              common::Buffer data) {
    // TODO peer ratings according to status

    if (!started_) {
      return;
    }

    block_cb_(std::move(cid), std::move(data));
  }

  void GraphsyncImpl::onRemoteRequest(const PeerId &from,
                                      Message::Request request) {
    // TODO make this asynchronous

    ResponseMetadata metadata;
    bool send_response = true;

    auto data_handler = [&](const CID &cid,
                            const common::Buffer &data) -> bool {
      bool data_present = !data.empty();

      if (request.send_metadata) {
        metadata.push_back({cid, data_present});
      }

      if (data_present) {
        if (!network_->addBlockToResponse(from, request.id, cid, data)) {
          send_response = false;
          return false;
        }
      }

      return true;
    };

    auto select_res =
        dag_->select(request.root_cid, request.selector, data_handler);

    if (!send_response) {
      // ignore response due to network side
      return;
    }

    ResponseStatusCode status = RS_NOT_FOUND;
    if (select_res && select_res.value() > 0) {
      status = RS_FULL_CONTENT;
    } else {
      status = RS_REQUEST_FAILED;
    }

    network_->sendResponse(from, request.id, status, metadata);
  }

  void GraphsyncImpl::cancelLocalRequest(RequestId request_id, SharedData body) {
    network_->cancelRequest(request_id, std::move(body));
  }

}  // namespace fc::storage::ipfs::graphsync
