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
    static size_t requests_sent;
    static size_t responses_received;

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
      ++requests_sent;
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
        ++responses_received;
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

  size_t Node::requests_sent = 0;
  size_t Node::responses_received = 0;

  void testTwoNodesClientServer() {
    Node::requests_sent = 0;
    Node::responses_received = 0;

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

    logger->info("total requests sent {}, responses received {}",
                 Node::requests_sent,
                 Node::responses_received);

    EXPECT_EQ(client_data->getReceived(), client_data->getExpected());
    EXPECT_EQ(unexpected, 0);
  }

  struct NodeParams {
    boost::optional<libp2p::multi::Multiaddress> listen_to;
    std::shared_ptr<TestDataService> data_service;
    std::vector<std::string> strings;
    boost::optional<libp2p::peer::PeerId> peer;
  };

  void testManyNodesExchange(size_t N, size_t n_data) {
    Node::requests_sent = 0;
    Node::responses_received = 0;

    size_t unexpected_responses = 0;
    size_t total_responses = 0;
    size_t expected = 0;

    std::vector<NodeParams> params;
    params.reserve(N);
    for (size_t i = 0; i < N; ++i) {
      auto &p = params.emplace_back();
      p.listen_to = libp2p::multi::Multiaddress::create(
                        fmt::format("/ip4/127.0.0.1/tcp/{}", 40000 + i))
                        .value();

      p.data_service = std::make_shared<TestDataService>();

      p.strings.reserve(n_data);
      for (size_t j = 0; j < n_data; ++j) {
        auto &s = p.strings.emplace_back(fmt::format("data_{}_{}", i, j));
        p.data_service->addData(s);
      }
    }

    auto io = std::make_shared<boost::asio::io_context>();

    std::vector<Node> nodes;
    nodes.reserve(N);

    for (size_t i = 0; i < N; ++i) {
      auto &p = params[i];

      auto cb = [ds = p.data_service,
                 &expected,
                 &unexpected_responses,
                 &total_responses,
                 &io](CID cid, common::Buffer data) {
        logger->trace("data block received, {}:{}, {}/{}",
                      cid.toString().value(),
                      std::string((const char *)data.data(), data.size()),
                      total_responses + 1,
                      expected);
        if (!ds->onDataBlock(std::move(cid), std::move(data))) {
          ++unexpected_responses;
        } else if (++total_responses == expected) {
          io->stop();
        }
      };

      auto &n = nodes.emplace_back(io, p.data_service, cb, 0);

      p.peer = n.getId();

      for (size_t j = 0; j < N; ++j) {
        if (j != i) {
          for (const auto &s : params[j].strings)
            p.data_service->addExpected(s);
        }
      }
    }

    io->post([&]() {
      for (size_t i = 0; i < N; ++i) {
        auto &p = params[i];
        auto &n = nodes[i];
        n.listen(p.listen_to.value());
      }

      io->post([&]() {
        for (size_t i = 0; i < N; ++i) {
          auto &p = params[i];
          auto &n = nodes[i];

          for (const auto &[cid, d] : p.data_service->getExpected()) {
            ++expected;
            for (const auto &p0 : params) {
              if (&p0 != &p) {
                logger->trace("request from {} to {} for {}:{}",
                              p.peer->toBase58().substr(46),
                              p0.peer->toBase58().substr(46),
                              cid.toString().value(),
                              std::string((const char *)d.data(), d.size()));
                n.makeRequest(p0.peer.value(), p0.listen_to, cid);
              }
            }
          }
        }
      });
    });

    runEventLoop(io, run_time_msec);

    logger->info("total requests sent {}, responses received {}",
                 Node::requests_sent,
                 Node::responses_received);

    EXPECT_EQ(unexpected_responses, 0);
    for (const auto &p : params) {
      EXPECT_EQ(p.data_service->getReceived(), p.data_service->getExpected());
    }
  }  // namespace fc::storage::ipfs::graphsync::test

}  // namespace fc::storage::ipfs::graphsync::test

TEST(GraphsyncAcceptance, TwoNodesClientServer) {
  namespace test = fc::storage::ipfs::graphsync::test;

  // test::testTwoNodesClientServer(/*param*/);
}

TEST(GraphsyncAcceptance, TwoNodesMutualExchange) {
  namespace test = fc::storage::ipfs::graphsync::test;

  // test::testManyNodesExchange(2, 1);
}

TEST(GraphsyncAcceptance, ManyNodesMutualExchange) {
  namespace test = fc::storage::ipfs::graphsync::test;

  test::testManyNodesExchange(7, 2);
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
