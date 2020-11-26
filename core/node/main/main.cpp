/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <libp2p/host/host.hpp>

#include "builder.hpp"
#include "common/logger.hpp"
#include "node/blocksync_client.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/events.hpp"
#include "node/identify.hpp"
#include "node/pubsub_gate.hpp"
#include "node/pubsub_workaround.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/syncer.hpp"
#include "node/tipset_loader.hpp"

namespace fc {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger.get();
    }

    void suppressVerboseLoggers() {
      common::createLogger("SECCONN")->set_level(spdlog::level::info);
      common::createLogger("SECIO")->set_level(spdlog::level::info);
      common::createLogger("tls")->set_level(spdlog::level::info);
      common::createLogger("gossip")->set_level(spdlog::level::info);
    }

  }  // namespace

  int main(int argc, char *argv[]) {
    node::Config config;

    if (!config.init(argc, argv)) {
      return __LINE__;
    }

    if (config.log_level <= spdlog::level::debug) {
      suppressVerboseLoggers();
    }

    auto obj_res = node::createNodeObjects(config);
    if (!obj_res) {
      log()->error("Cannot initialize node: {}", obj_res.error().message());
      return __LINE__;
    }
    auto &o = obj_res.value();

    log()->info("Starting components");

    auto events = std::make_shared<sync::events::Events>(o.scheduler);

    bool started = false;

    node::PubsubWorkaround pubsub2(o.io_context,
                                   config.bootstrap_list,
                                   config.gossip_config,
                                   config.network_name);

    // will start listening/connecting only after current chain head is set
    auto conn = events->subscribeCurrentHead(
        [&](const sync::events::CurrentHead &head) {
          log()->info(
              "\n============================ {} ============================",
              head.tipset->height());

          if (started) {
            return;
          }

          started = true;

          if (auto r = o.host->listen(config.listen_address); !r) {
            log()->error("Cannot listen to {}: {}",
                         config.listen_address.getStringAddress(),
                         r.error().message());
            o.io_context->stop();
            return;
          }

          o.host->start();
          auto peer_info = o.host->getPeerInfo();
          if (peer_info.addresses.empty()) {
            log()->error("Cannot listen to {}: {}",
                         config.listen_address.getStringAddress());
            o.io_context->stop();
            return;
          }

          log()->info("Node started at /ip4/{}/tcp/{}/p2p/{}",
                      config.local_ip,
                      config.port,
                      peer_info.id.toBase58());

          for (const auto &pi : config.bootstrap_list) {
            o.host->connect(pi);
          }

          auto p2_res = pubsub2.start(config.port + 1);
          if (!p2_res) {
            log()->warn("cannot start pubsub workaround, {}",
                        p2_res.error().message());
          } else {
            o.gossip->addBootstrapPeer(p2_res.value().id,
                                       p2_res.value().addresses[0]);
          }
        });

    o.identify->start(events);
    o.say_hello->start(config.genesis_cid.value(), events);
    o.receive_hello->start(config.genesis_cid.value(), events);
    o.gossip->start();
    o.pubsub_gate->start(config.network_name, events);
    // o.graphsync->start();
    o.blocksync_server->start();
    o.blocksync_client->start(events);
    o.tipset_loader->start(events);
    o.syncer->start(events);

    // chain store starts after all other components, it chooses current head
    // and emits possible heads
    if (auto r = o.chain_store->start(events, config.network_name); !r) {
      log()->error("Cannot start node: {}", r.error().message());
      return __LINE__;
    }

    // gracefully shutdown on signal
    boost::asio::signal_set signals(*o.io_context, SIGINT, SIGTERM);
    signals.async_wait(
        [&](const boost::system::error_code &, int) { o.io_context->stop(); });

    // run event loop
    o.io_context->run();
    log()->info("Node stopped");

    return 0;
  }

}  // namespace fc

int main(int argc, char *argv[]) {
#ifdef NDEBUG
  try {
    return fc::main(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "UNEXPECTED EXCEPTION, " << e.what() << "\n";
  } catch (...) {
    std::cerr << "UNEXPECTED EXCEPTION\n";
  }
  return __LINE__;
#else
  return fc::main(argc, argv);
#endif
}
