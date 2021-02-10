/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include <libp2p/host/host.hpp>

#include "api/rpc/info.hpp"
#include "api/rpc/ws.hpp"
#include "common/logger.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/interpret_job.hpp"
#include "node/main/builder.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/pubsub_workaround.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "vm/actor/cgo/actors.hpp"

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
      common::createLogger("gossip")->set_level(spdlog::level::warn);
      common::createLogger("kad")->set_level(spdlog::level::info);
      common::createLogger("IdentifyMsgProcessor")
          ->set_level(spdlog::level::warn);
      common::createLogger("NoiseHandshake")->set_level(spdlog::level::warn);
      common::createLogger("Noise")->set_level(spdlog::level::warn);
      common::createLogger("yx-conn")->set_level(spdlog::level::critical);
      common::createLogger("pubsub-2")->set_level(spdlog::level::info);
      common::createLogger("pubsub_gate")->set_level(spdlog::level::info);
      common::createLogger("say_hello")->set_level(spdlog::level::info);
      common::createLogger("peer_discovery")->set_level(spdlog::level::info);
      common::createLogger("identify")->set_level(spdlog::level::info);
    }

  }  // namespace

  int main(int argc, char *argv[]) {
    vm::actor::cgo::configMainnet();

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

    auto events = o.events;

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

          auto p2_res = pubsub2.start(config.port == -1 ? -1 : config.port + 2);
          if (!p2_res) {
            log()->warn("cannot start pubsub workaround, {}",
                        p2_res.error().message());
          } else {
            o.gossip->addBootstrapPeer(p2_res.value().id,
                                       p2_res.value().addresses[0]);
          }

          auto routes{std::make_shared<api::Routes>()};
          api::serve(
              o.api, routes, *o.io_context, "127.0.0.1", config.api_port);
          api::rpc::saveInfo(config.repo_path, config.api_port, "stub");
          log()->info("API started at ws://127.0.0.1:{}", config.api_port);
        });

    o.identify->start(events);
    o.say_hello->start(config.genesis_cid.value(), events);
    o.receive_hello->start();
    o.gossip->start();
    o.pubsub_gate->start(config.network_name, events);
    o.graphsync_server->start();
    o.blocksync_server->start();
    o.sync_job->start(events);
    o.peer_discovery->start(*events);

    bool fatal_error_occured = false;
    auto fatal_error_conn =
        events->subscribeFatalError([&](const sync::events::FatalError &e) {
          fatal_error_occured = true;
          o.io_context->stop();
          log()->error("Fatal error: {}", e.message);
        });

    // chain store starts after all other components, it chooses current head
    // and emits possible heads
    if (auto r = o.chain_store->start(events); !r) {
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

    return fatal_error_occured ? __LINE__ : 0;
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
