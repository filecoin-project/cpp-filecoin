/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/graphsync_server.hpp"

#include "common/hexutil.hpp"
#include "common/logger.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"
#include "storage/ipld/traverser.hpp"

namespace fc::sync {

  namespace gs = storage::ipfs::graphsync;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("graphsync_server");
      return logger.get();
    }

    gs::Response handleRequest(Ipld &ipld, gs::Request request) {
      gs::Response response;

      log()->debug("got new request with selector: {}",
                   common::hex_lower(request.selector));

      storage::ipld::traverser::Traverser traverser{
          ipld, request.root_cid, {request.selector}, true};
      auto ok{true};
      if (auto _cids{traverser.traverseAll()}) {
        for (auto &cid : _cids.value()) {
          if (auto _data{ipld.get(cid)}) {
            response.data.push_back({std::move(cid), std::move(_data.value())});
          } else {
            ok = false;
            break;
          }
        }
      }
      response.status = ok ? gs::RS_FULL_CONTENT : gs::RS_INTERNAL_ERROR;

      return response;
    }

  }  // namespace

  GraphsyncServer::GraphsyncServer(
      std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync,
      IpldPtr ipld)
      : graphsync_(std::move(graphsync)), ipld_(std::move(ipld)) {
    assert(graphsync_);
    assert(ipld_);
  }

  void GraphsyncServer::start() {
    if (!started_) {
      graphsync_->setDefaultRequestHandler(
          [this](gs::FullRequestId id, gs::Request request) {
            gs::Response response = handleRequest(*ipld_, std::move(request));

            // this may be done asynchronously as well!
            graphsync_->postResponse(id, response);
          });
      graphsync_->start();
      started_ = true;
      log()->debug("started");
    }
  }

}  // namespace fc::sync
