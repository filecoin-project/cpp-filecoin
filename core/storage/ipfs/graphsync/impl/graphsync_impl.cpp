/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_impl.hpp"

#include <cassert>

#include "local_requests.hpp"

namespace fc::storage::ipfs::graphsync {

  void GraphsyncImpl::start(std::shared_ptr<MerkleDagBridge> dag,
                            Graphsync::BlockCallback callback) {
    assert(dag);
    assert(callback);

    network_->start(shared_from_this());
    dag_ = std::move(dag);
    block_cb_ = std::move(callback);
    started_ = true;
  }

  Subscription GraphsyncImpl::makeRequest(
      const libp2p::peer::PeerId &peer,
      boost::optional<libp2p::multi::Multiaddress> address,
      const CID& root_cid,
      gsl::span<const uint8_t> selector,
      bool need_metadata,
      const std::vector<CID> &dont_send_cids,
      RequestProgressCallback callback) {
    if (!started_) {
      // TODO defer RS_GRAPHSYNC_STOPPED to callback
      return Subscription();
    }

    auto [subscr, request_id] =
        local_requests_->newRequest(std::move(callback));

    if (request_id == 0) {
      // TODO defer RS_INtERNAL_ERROR
      return std::move(subscr);
    }

    request_builder_.addRequest(
        request_id, root_cid, selector, need_metadata, dont_send_cids);

    auto serialize_res = request_builder_.serialize();

    request_builder_.clear();

    if (serialize_res) {
      network_->makeRequest(peer,
                            std::move(address),
                            request_id,
                            std::move(serialize_res.value()));
    } else {
      // defer serialize error
    }

    return std::move(subscr);
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
                                      uint64_t tag,
                                      Message::Request request) {
    // TODO make this asynchronous

    ResponseMetadata metadata;

    auto data_handler = [&](const CID &cid,
                            const common::Buffer &data) -> bool {
      bool data_present = !data.empty();

      if (request.send_metadata) {
        metadata.push_back({cid, data_present});
      }

      if (data_present) {
        network_->addBlockToResponse(from, tag, cid, data);
      }

      return true;
    };

    auto select_res =
        dag_->select(request.root_cid, request.selector, data_handler);

    ResponseStatusCode status = RS_NOT_FOUND;
    if (select_res && select_res.value() > 0) {
      status = RS_FULL_CONTENT;
    } else {
      status = RS_REQUEST_FAILED;
    }

    network_->sendResponse(from, tag, request.id, status, metadata);
  }

  void GraphsyncImpl::cancelLocalRequest(int request_id) {
    if (!started_) {
      return;
    }

    request_builder_.addCancelRequest(request_id);
    auto serialize_res = request_builder_.serialize();
    request_builder_.clear();

    network_->cancelRequest(
        request_id,
        serialize_res ? std::move(serialize_res.value()) : SharedData{});
  }

}  // namespace fc::storage::ipfs::graphsync
