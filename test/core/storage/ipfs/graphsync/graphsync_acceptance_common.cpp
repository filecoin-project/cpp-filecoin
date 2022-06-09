/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphsync_acceptance_common.hpp"

#include <boost/di/extension/scopes/shared.hpp>

#include <libp2p/injector/host_injector.hpp>

#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"

namespace fc::storage::ipfs::graphsync::test {

  void runEventLoop(const std::shared_ptr<boost::asio::io_context> &io,
                    size_t max_milliseconds) {
    if (max_milliseconds > 0) {
      io->run_for(std::chrono::milliseconds(max_milliseconds));
    } else {
      io->run();
    }
  }

  std::pair<std::shared_ptr<Graphsync>, std::shared_ptr<libp2p::Host>>
  createNodeObjects(std::shared_ptr<boost::asio::io_context> io) {
    // [boost::di::override] allows for creating multiple hosts for testing
    // purposes
    auto injector =
        libp2p::injector::makeHostInjector<boost::di::extension::shared_config>(
            boost::di::bind<boost::asio::io_context>.to(
                io)[boost::di::override]);

    std::pair<std::shared_ptr<Graphsync>, std::shared_ptr<libp2p::Host>>
        objects;
    objects.second = injector.template create<std::shared_ptr<libp2p::Host>>();
    auto scheduler =
        injector.create<std::shared_ptr<libp2p::basic::Scheduler>>();
    objects.first =
        std::make_shared<GraphsyncImpl>(objects.second, std::move(scheduler));
    return objects;
  }

  bool TestDataService::onDataBlock(const Data &data) {
    bool expected = false;
    auto it = expected_.find(data.cid);
    if (it != expected_.end() && it->second == data.content) {
      expected = (received_.count(data.cid) == 0);
    }
    received_[data.cid] = data.content;
    return expected;
  }

  void TestDataService::insertNode(TestDataService::Storage &dst,
                                   const std::string &data_str) {
    auto bytes{common::span::cbytes(data_str)};
    dst[common::getCidOf(bytes).value()] = copy(bytes);
  }

  outcome::result<size_t> TestDataService::select(
      const CID &cid,
      gsl::span<const uint8_t> selector,
      std::function<bool(const CID &cid, const Bytes &data)> handler) const {
    auto it = data_.find(cid);
    if (it != data_.end()) {
      handler(it->first, it->second);
      return 1;
    }
    return 0;
  }

}  // namespace fc::storage::ipfs::graphsync::test
