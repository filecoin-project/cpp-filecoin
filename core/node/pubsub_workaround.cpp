/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/pubsub_workaround.hpp"

#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>

#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/logger.hpp"
#include "crypto/blake2/blake2b160.hpp"

namespace fc::node {
  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("pubsub-2");
      return logger.get();
    }
  }  // namespace

  class PubsubWorkaround::Impl {
   public:
    Impl(std::shared_ptr<libp2p::Host> host,
         std::shared_ptr<libp2p::protocol::gossip::Gossip> gossip,
         std::string network_name)
        : host_(std::move(host)),
          gossip_(std::move(gossip)),
          network_name_(std::move(network_name)) {}

    ~Impl() {
      if (started_) {
        msgs_subscription_.cancel();
        blocks_subscription_.cancel();
        gossip_->stop();
        host_->stop();
        log()->info("stopped");
      }
    }

    outcome::result<libp2p::peer::PeerInfo> start(int port) {
      if (started_) {
        log()->warn("already started");
        return host_->getPeerInfo();
      }

      OUTCOME_TRY(addr,
                  libp2p::multi::Multiaddress::create(
                      fmt::format("/ip4/127.0.0.1/tcp/{}", port)));
      OUTCOME_TRY(host_->listen(addr));
      host_->start();

      using libp2p::protocol::gossip::ByteArray;
      gossip_->setMessageIdFn([](const ByteArray &from,
                                 const ByteArray &seq,
                                 const ByteArray &data) {
        auto h = crypto::blake2b::blake2b_256(data);
        return ByteArray(h.data(), h.data() + h.size());
      });

      auto topic = std::string("/fil/blocks/") + network_name_;
      blocks_subscription_ = gossip_->subscribe({topic}, [](auto m) {
        if (m) {
          log()->debug("got new block via pubsub");
        }
      });

      topic = std::string("/fil/msgs/") + network_name_;
      msgs_subscription_ = gossip_->subscribe({topic}, [](auto m) {
        if (m) {
          log()->debug("got new msg via pubsub");
        }
      });

      gossip_->start();

      auto pi = host_->getPeerInfo();

      log()->info("started at {}/p2p/{}",
                  nonZeroAddr(pi.addresses)->getStringAddress(),
                  pi.id.toBase58());

      started_ = true;
      return pi;
    }

   private:
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<libp2p::protocol::gossip::Gossip> gossip_;
    std::string network_name_;
    libp2p::protocol::Subscription blocks_subscription_;
    libp2p::protocol::Subscription msgs_subscription_;
    bool started_ = false;
  };

  PubsubWorkaround::PubsubWorkaround(
      std::shared_ptr<boost::asio::io_context> io_context,
      const std::vector<libp2p::peer::PeerInfo> &bootstrap_list,
      const libp2p::protocol::gossip::Config &gossip_config,
      std::string network_name) {
    auto injector =
        libp2p::injector::makeHostInjector<boost::di::extension::shared_config>(
            boost::di::bind<boost::asio::io_context>.template to(
                io_context)[boost::di::override]);
    auto host = injector.create<std::shared_ptr<libp2p::Host>>();
    auto gossip = libp2p::protocol::gossip::create(
        injector.create<std::shared_ptr<libp2p::basic::Scheduler>>(),
        host,
        gossip_config);
    for (const auto &b : bootstrap_list) {
      gossip->addBootstrapPeer(b.id, b.addresses[0]);
    }
    impl_ = std::make_shared<Impl>(
        std::move(host), std::move(gossip), std::move(network_name));
  }

  outcome::result<libp2p::peer::PeerInfo> PubsubWorkaround::start(int port) {
    return impl_->start(port);
  }

}  // namespace fc::node
