/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/fmt/fmt.h>

#include "common/logger.hpp"
#include "graphsync_acceptance_common.hpp"

static std::shared_ptr<spdlog::logger> logger;
static size_t run_time_msec = 2000;

namespace fc::storage::ipfs::graphsync::test {

  class Node {
   public:
    Node(std::shared_ptr<boost::asio::io_context> io,
         std::shared_ptr<MerkleDagBridge> data_service,
         Graphsync::BlockCallback cb,
         size_t n_responses_expected)
        : io_(std::move(io)),
          data_service_(std::move(data_service)),
          block_cb_(std::move(cb)),
          n_responses_expected_(n_responses_expected) {
      std::tie(graphsync_, host_) = createNodeObjects(io_);
    }

    auto getId() const {
      return host_->getId();
    }

    void listen(const libp2p::multi::Multiaddress &listen_to) {
      auto listen_res = host_->listen(listen_to);
      if (!listen_res) {
        logger->trace("Cannot listen to multiaddress {}, {}",
                      listen_to.getStringAddress(),
                      listen_res.error().message());
        return;
      }
      start();
    }

    void makeRequest(const libp2p::peer::PeerId &peer,
                     boost::optional<libp2p::multi::Multiaddress> address,
                     const CID &root_cid) {
      start();
      requests_.push_back(graphsync_->makeRequest(peer,
                                                  std::move(address),
                                                  root_cid,
                                                  {},
                                                  true,
                                                  {},
                                                  requestProgressCallback()));
    }

   private:
    void start() {
      if (!started_) {
        graphsync_->start(data_service_, block_cb_);
        host_->start();
        started_ = true;
      }
    }

    Graphsync::RequestProgressCallback requestProgressCallback() {
      static auto formatMeta = [](const ResponseMetadata &meta) -> std::string {
        std::string s;
        for (const auto &item : meta) {
          s += fmt::format(
              "({}:{}) ", item.first.toString().value(), item.second);
        }
        return s;
      };
      return [this](ResponseStatusCode code, ResponseMetadata meta) {
        logger->trace(
            "request progress: code={}, meta={}", code, formatMeta(meta));
        if (++n_responses == n_responses_expected_) {
          io_->stop();
        }
      };
    }

    std::shared_ptr<boost::asio::io_context> io_;
    std::shared_ptr<Graphsync> graphsync_;
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<MerkleDagBridge> data_service_;
    Graphsync::BlockCallback block_cb_;
    std::vector<Subscription> requests_;
    size_t n_responses_expected_;
    size_t n_responses = 0;
    bool started_ = false;
  };

  void testTwoNodesExchange(/*param*/) {
    auto listen_to =
        libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40000").value();

    auto io = std::make_shared<boost::asio::io_context>();

    std::vector<std::string> strings({"xxx", "yyy", "zzz"});

    size_t unexpected = 0;

    auto server_data = std::make_shared<TestDataService>();
    auto server_cb = [&unexpected](CID, common::Buffer) { ++unexpected; };

    auto client_data = std::make_shared<TestDataService>();
    auto client_cb = [&client_data, &unexpected](CID cid, common::Buffer data) {
      if (!client_data->onDataBlock(std::move(cid), std::move(data))) {
        ++unexpected;
      }
    };

    for (const auto &s : strings) {
      server_data->addData(s);
      client_data->addExpected(s);
    }

    Node server(io, server_data, server_cb, 0);
    Node client(io, client_data, client_cb, 3);

    io->post([&]() {
      server.listen(listen_to);
      auto peer = server.getId();
      bool use_address = true;
      for (const auto &[cid, _] : client_data->getExpected()) {
        boost::optional<libp2p::multi::Multiaddress> address(listen_to);
        client.makeRequest(peer, use_address ? address : boost::none, cid);
        use_address = false;
      }
    });

    runEventLoop(io, run_time_msec);

    EXPECT_EQ(client_data->getData(), client_data->getExpected());
    EXPECT_EQ(unexpected, 0);
  }

}  // namespace fc::storage::ipfs::graphsync::test

TEST(GraphsyncAcceptance, TwoNodesExchange) {
  namespace test = fc::storage::ipfs::graphsync::test;

  test::testTwoNodesExchange(/*param*/);
}

int main(int argc, char *argv[]) {
  logger = fc::common::createLogger("test");
  if (argc > 1 && std::string("trace") == argv[1]) {
    logger->set_level(spdlog::level::trace);
    fc::common::createLogger("graphsync")->set_level(spdlog::level::trace);
    run_time_msec = 1000000000;
    --argc;
    ++argv;
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
