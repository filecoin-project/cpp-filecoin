/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_impl.hpp"

#include <cassert>

#include "local_requests.hpp"
#include "network/network.hpp"

namespace fc::storage::ipfs::graphsync {
  /// Selector that matches current node
  Bytes kSelectorMatcher{0xa1, 0x61, 0x2e, 0xa0};

  GraphsyncImpl::GraphsyncImpl(std::shared_ptr<libp2p::Host> host,
                               std::shared_ptr<Scheduler> scheduler)
      : scheduler_(scheduler),
        network_(std::make_shared<Network>(std::move(host), scheduler)),
        local_requests_(std::make_shared<LocalRequests>(
            std::move(scheduler),
            [this](RequestId request_id, SharedData body) {
              cancelLocalRequest(request_id, std::move(body));
            })) {}

  GraphsyncImpl::~GraphsyncImpl() {
    doStop();
  }

  Graphsync::DataConnection GraphsyncImpl::subscribe(
      std::function<OnDataReceived> handler) {
    return data_signal_.connect(handler);
  }

  void GraphsyncImpl::setDefaultRequestHandler(
      std::function<RequestHandler> handler) {
    assert(handler);
    if (default_request_handler_) {
      logger()->warn("overriding default request handler");
    }
    default_request_handler_ = std::move(handler);
  }

  void GraphsyncImpl::setRequestHandler(std::function<RequestHandler> handler,
                                        std::string extension_name) {
    assert(handler);
    assert(!extension_name.empty());
    if (extension_name.empty()) {
      return;
    }
    if (request_handlers_.count(extension_name) != 0) {
      logger()->warn("overriding request handler for extension ");
    }
    request_handlers_.insert({std::move(extension_name), std::move(handler)});
  }

  void GraphsyncImpl::postResponse(const FullRequestId &id,
                                   const Response &response) {
    network_->sendResponse(id, response);
  }

  void GraphsyncImpl::start() {
    if (started_) {
      return;
    }

    if (!default_request_handler_) {
      logger()->warn("default request handler not set");
    }

    network_->start(shared_from_this());
    started_ = true;
  }

  void GraphsyncImpl::stop() {
    doStop();
  }

  void GraphsyncImpl::doStop() {
    if (started_) {
      started_ = false;
      default_request_handler_ = decltype(default_request_handler_){};
      request_handlers_.clear();
      network_->stop();
      local_requests_->cancelAll();
    }
  }

  Subscription GraphsyncImpl::makeRequest(
      const libp2p::peer::PeerInfo &peer,
      const CID &root_cid,
      gsl::span<const uint8_t> selector,
      const std::vector<Extension> &extensions,
      RequestProgressCallback callback) {
    if (!started_ || !network_->canSendRequest(peer.id)) {
      logger()->trace("makeRequest: rejecting request to peer {}",
                      peer.id.toBase58().substr(46));
      return local_requests_->newRejectedRequest(std::move(callback));
    }

    if (selector.empty()) {
      selector = kSelectorMatcher;
    }

    auto newRequest = local_requests_->newRequest(
        root_cid, selector, extensions, std::move(callback));

    if (newRequest.request_id > 0) {
      assert(newRequest.body);
      assert(!newRequest.body->empty());

      logger()->trace("makeRequest: sending request to peer {}",
                      peer.id.toBase58().substr(46));

      network_->makeRequest(
          peer, newRequest.request_id, std::move(newRequest.body));
    }

    return std::move(newRequest.subscription);
  }

  void GraphsyncImpl::onResponse(const PeerId &peer,
                                 int request_id,
                                 ResponseStatusCode status,
                                 std::vector<Extension> extensions) {
    if (!started_) {
      return;
    }

    local_requests_->onResponse(request_id, status, std::move(extensions));
  }

  void GraphsyncImpl::onDataBlock(const PeerId &from, Data data) {
    if (!started_) {
      return;
    }

    data_signal_(from, std::move(data));
  }

  void GraphsyncImpl::onRemoteRequest(const PeerId &from,
                                      Message::Request request) {
    auto call_request =
        [](const PeerId &from, Message::Request request, const auto &handler) {
          handler(FullRequestId{from, request.id},
                  Request{std::move(request.root_cid),
                          std::move(request.selector),
                          std::move(request.extensions),
                          request.cancel});
        };

    if (!request.extensions.empty() && !request_handlers_.empty()) {
      for (const auto &[ext_name, handler] : request_handlers_) {
        if (Extension::find(ext_name, request.extensions)) {
          call_request(from, std::move(request), handler);
          return;
        }
      }
    }

    if (!default_request_handler_) {
      postResponse(FullRequestId{from, request.id},
                   Response{RS_REJECTED, {}, {}});
      return;
    }

    call_request(from, std::move(request), default_request_handler_);
  }

  void GraphsyncImpl::cancelLocalRequest(RequestId request_id,
                                         SharedData body) {
    network_->cancelRequest(request_id, std::move(body));
  }

}  // namespace fc::storage::ipfs::graphsync
