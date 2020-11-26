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
      return 1;
    }

    if (config.log_level <= spdlog::level::debug) {
      suppressVerboseLoggers();
    }

    auto res = node::createNodeObjects(config);
    if (!res) {
      log()->error("Cannot initialize node: {}", res.error().message());
      return 2;
    }

    auto &o = res.value();

    log()->info("Starting components");

    auto events = std::make_shared<sync::events::Events>(o.scheduler);

    // will start listening/connecting only after current chain head is set
    sync::events::Connection network_start = events->subscribeCurrentHead(
        [&](const sync::events::CurrentHead &head) {
          o.host->start();
          for (const auto &pi : config.bootstrap_list) {
            o.host->connect(pi);
          }

          // this is one-shot event
          events->CurrentHead_signal_.disconnect(network_start);
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

    // chain store starts after all, it chooses current head and emits possible
    // heads
    auto started = o.chain_store->start(events, config.network_name);
    if (!started) {
      log()->error("Cannot start node: {}", started.error().message());
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
  try {
    return fc::main(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "UNEXPECTED EXCEPTION, " << e.what() << "\n";
  } catch (...) {
    std::cerr << "UNEXPECTED EXCEPTION\n";
  }
  return 127;
}
