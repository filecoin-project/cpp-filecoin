/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/fmt/fmt.h>

#include "common/logger.hpp"
#include "graphsync_acceptance_common.hpp"

// logger used by these tests
static std::shared_ptr<spdlog::logger> logger;

// max test case run time, always limited by various params
static size_t run_time_msec = 0;

namespace fc::storage::ipfs::graphsync::test {

  // Test node aggregate
  class Node {
   public:
    // total requests sent by all nodes in a test case
    static size_t requests_sent;

    // total responses received by all nodes in a test case
    static size_t responses_received;

    // n_responses_expected: count of responses received by the node after which
    // io->stop() is called
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

    // stops graphsync and host, otherwise they can interact with further tests!
    void stop() {
      graphsync_->stop();
      host_->stop();
    }

    // returns peer ID, so they can connect to each other
    auto getId() const {
      return host_->getId();
    }

    // listens to network and starts nodes if not yet started
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

    // calls Graphsync's makeRequest
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

    // helper, returns requesu callback fn
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
            "request progress: code={}, meta={}", statusCodeToString(code), formatMeta(meta));
        if (++n_responses == n_responses_expected_) {
          io_->stop();
        }
      };
    }

    // asion context to be stopped when needed
    std::shared_ptr<boost::asio::io_context> io_;

    std::shared_ptr<Graphsync> graphsync_;

    std::shared_ptr<libp2p::Host> host_;

    std::shared_ptr<MerkleDagBridge> data_service_;

    Graphsync::BlockCallback block_cb_;

    // keeping subscriptions alive, otherwise they cancel themselves
    std::vector<Subscription> requests_;

    size_t n_responses_expected_;
    size_t n_responses = 0;
    bool started_ = false;
  };

  size_t Node::requests_sent = 0;
  size_t Node::responses_received = 0;

  // Two nodes interact, one connection is utilized
  void testTwoNodesClientServer() {
    Node::requests_sent = 0;
    Node::responses_received = 0;

    auto listen_to =
        libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40000").value();

    auto io = std::make_shared<boost::asio::io_context>();

    // strings from which we create blocks and CIDs
    std::vector<std::string> strings({"xxx", "yyy", "zzz"});

    size_t unexpected = 0;

    // creating instances

    auto server_data = std::make_shared<TestDataService>();

    // server block callback expects no blocks
    auto server_cb = [&unexpected](CID, common::Buffer) { ++unexpected; };

    auto client_data = std::make_shared<TestDataService>();

    // clienc block callback expect 3 blocks from the string above
    auto client_cb = [&client_data, &unexpected](CID cid, common::Buffer data) {
      if (!client_data->onDataBlock(std::move(cid), std::move(data))) {
        ++unexpected;
      }
    };

    for (const auto &s : strings) {

      // client expects what server has

      server_data->addData(s);
      client_data->addExpected(s);
    }

    Node server(io, server_data, server_cb, 0);
    Node client(io, client_data, client_cb, 3);

    // starting all the stuff asynchronously

    io->post([&]() {
      // server listens
      server.listen(listen_to);
      auto peer = server.getId();
      bool use_address = true;

      // client makes 3 requests

      for (const auto &[cid, _] : client_data->getExpected()) {
        boost::optional<libp2p::multi::Multiaddress> address(listen_to);

        // don't need to pass the address more than once
        client.makeRequest(peer, use_address ? address : boost::none, cid);
        use_address = false;
      }
    });

    runEventLoop(io, run_time_msec);

    client.stop();
    server.stop();

    logger->info("total requests sent {}, responses received {}",
                 Node::requests_sent,
                 Node::responses_received);

    EXPECT_EQ(client_data->getReceived(), client_data->getExpected());
    EXPECT_EQ(unexpected, 0);
  }

  // Context for more complex cases
  struct NodeParams {
    // listen address
    boost::optional<libp2p::multi::Multiaddress> listen_to;

    // MerkleDAG stub for node
    std::shared_ptr<TestDataService> data_service;

    // Strings to make blocks and CIDs from them
    std::vector<std::string> strings;

    // peer ID
    boost::optional<libp2p::peer::PeerId> peer;
  };

  // N nodes communicate P2P with each other  and collect many blocks.
  // Each node has n_data data blocks
  void testManyNodesExchange(size_t N, size_t n_data) {
    Node::requests_sent = 0;
    Node::responses_received = 0;

    size_t unexpected_responses = 0;
    size_t total_responses = 0;
    size_t expected = 0;

    // creating parameters for N nodes

    std::vector<NodeParams> params;
    params.reserve(N);
    for (size_t i = 0; i < N; ++i) {
      auto &p = params.emplace_back();

      // The node #i will listen to 40000+i pore
      p.listen_to = libp2p::multi::Multiaddress::create(
                        fmt::format("/ip4/127.0.0.1/tcp/{}", 40000 + i))
                        .value();

      p.data_service = std::make_shared<TestDataService>();

      // the i-th node has data represented by strings data_i_j, j in[0, n_data)
      p.strings.reserve(n_data);
      for (size_t j = 0; j < n_data; ++j) {
        auto &s = p.strings.emplace_back(fmt::format("data_{}_{}", i, j));
        p.data_service->addData(s);
      }
    }

    auto io = std::make_shared<boost::asio::io_context>();

    // creating N nodes

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

      // peer IDs are known only at this point
      p.peer = n.getId();

      for (size_t j = 0; j < N; ++j) {
        if (j != i) {
          for (const auto &s : params[j].strings)

            // each node expects data other hodes have
            p.data_service->addExpected(s);
        }
      }
    }

    // starting N nodes asynchronously

    io->post([&]() {
      for (size_t i = 0; i < N; ++i) {
        auto &p = params[i];
        auto &n = nodes[i];

        // each node listens
        n.listen(p.listen_to.value());
      }

      // will make connections in the next cycle
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

                // each node request every piece of expected data from
                // all other nodes. And gets RS_FULL_CONTENT 1 time per each
                // data block,
                // and respectively RS_NOT_FOUND will come N-2 times per block

                n.makeRequest(p0.peer.value(), p0.listen_to, cid);
              }
            }
          }
        }
      });
    });

    runEventLoop(io, run_time_msec);

    for (auto& n: nodes) {
      n.stop();
    }

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

  test::testTwoNodesClientServer(/*param*/);
}

TEST(GraphsyncAcceptance, TwoNodesMutualExchange) {
  namespace test = fc::storage::ipfs::graphsync::test;

  test::testManyNodesExchange(2, 1);
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
    --argc;
    ++argv;
  } else {
    spdlog::set_level(spdlog::level::err);
    run_time_msec = 900000;
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
