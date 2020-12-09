/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_server.hpp"

#include "common/hexutil.hpp"
#include "common/logger.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"
#include "storage/ipfs/merkledag/merkledag_service.hpp"

namespace fc::sync {

  namespace gs = storage::ipfs::graphsync;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("graphsync_server");
      return logger.get();
    }

    gs::Response handleRequest(storage::ipfs::merkledag::MerkleDagService &mds,
                               gs::Request request) {
      gs::Response response;

      log()->debug("got new request with selector: {}",
                   common::hex_lower(request.selector));

      try {
        auto handler =
            [&](std::shared_ptr<const storage::ipld::IPLDNode> node) {
              response.data.push_back(
                  gs::Data{node->getCID(), node->getRawBytes()});
              return true;
            };

        OUTCOME_EXCEPT(bytes, request.root_cid.toBytes());
        OUTCOME_EXCEPT(mds.select(bytes, request.selector, handler));
        response.status =
            response.data.empty() ? gs::RS_NOT_FOUND : gs::RS_FULL_CONTENT;

      } catch (const std::system_error &e) {
        log()->debug("request handler error {}", e.code().message());
        response.status = gs::RS_INTERNAL_ERROR;
      }

      return response;
    }

  }  // namespace

  GraphsyncServer::GraphsyncServer(
      std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync,
      std::shared_ptr<MerkleDagService> data_service)
      : graphsync_(std::move(graphsync)),
        data_service_(std::move(data_service)) {
    assert(graphsync_);
    assert(data_service_);
  }

  void GraphsyncServer::start() {
    if (!started_) {
      graphsync_->setDefaultRequestHandler(
          [this](gs::FullRequestId id, gs::Request request) {
            gs::Response response =
                handleRequest(*data_service_, std::move(request));

            // this may be done asynchronously as well!
            graphsync_->postResponse(id, response);
          });
      graphsync_->start();
      started_ = true;
      log()->debug("started");
    }
  }

}  // namespace fc::sync
