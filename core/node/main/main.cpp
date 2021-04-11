/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include "api/rpc/info.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/logger.hpp"
#include "common/soralog.hpp"
#include "drand/impl/http.hpp"
#include "markets/storage/types.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/main/builder.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/pubsub_workaround.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "vm/actor/cgo/actors.hpp"

namespace fc {
  using api::Import;
  using api::ImportRes;
  using markets::storage::StorageProviderInfo;
  using node::NodeObjects;
  using primitives::sector::getPreferredSealProofTypeFromWindowPoStType;

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

  void startApi(const node::Config &config, NodeObjects &node_objects) {
    // Network API
    PeerInfo api_peer_info{
        node_objects.host->getPeerInfo().id,
        nonZeroAddrs(node_objects.host->getAddresses(), &config.localIp())};
    node_objects.api->NetAddrsListen = [api_peer_info] {
      return api_peer_info;
    };
    node_objects.api->NetConnect = [&](auto &peer) {
      node_objects.host->connect(peer);
      return outcome::success();
    };
    node_objects.api->NetPeers = [&]() {
      const auto &peer_repository = node_objects.host->getPeerRepository();
      auto connections = node_objects.host->getNetwork()
                             .getConnectionManager()
                             .getConnections();
      std::vector<PeerInfo> result;
      for (const auto &conncection : connections) {
        auto remote = conncection->remotePeer();
        if (remote.has_error())
          log()->error("get remote peer error", remote.error().message());
        result.push_back(peer_repository.getPeerInfo(remote.value()));
      }
      return result;
    };

    // Market Client API
    node_objects.api->ClientImport =
        [&](auto &file_ref) -> outcome::result<ImportRes> {
      OUTCOME_TRY(root,
                  node_objects.storage_market_import_manager->import(
                      file_ref.path, file_ref.is_car));
      // storage id set to 0
      return ImportRes{root, 0};
    };
    node_objects.api->ClientStartDeal =
        [&](auto &params) -> outcome::result<CID> {
      // resolve wallet address and check if address exists in wallet
      OUTCOME_TRY(wallet_key,
                  node_objects.api->StateAccountKey(params.wallet, {}));
      OUTCOME_TRY(wallet_exists, node_objects.api->WalletHas(wallet_key));
      if (!wallet_exists) {
        return ERROR_TEXT("Node API: provided address doesn't exist in wallet");
      }

      OUTCOME_TRY(miner_info,
                  node_objects.api->StateMinerInfo(params.miner, {}));

      OUTCOME_TRY(peer_id, PeerId::fromBytes(miner_info.peer_id));
      const PeerInfo peer_info{.id = peer_id,
                               .addresses = miner_info.multiaddrs};
      StorageProviderInfo provider_info{.address = params.miner,
                                        .owner = {},
                                        .worker = miner_info.worker,
                                        .sector_size = miner_info.sector_size,
                                        .peer_info = peer_info};

      auto start_epoch = params.deal_start_epoch;
      if (start_epoch <= 0) {
        static const size_t kDealStartBufferHours = 49;
        OUTCOME_TRY(chain_head, node_objects.api->ChainHead());
        start_epoch =
            chain_head->height() + kDealStartBufferHours * kEpochsInHour;
      }

      OUTCOME_TRY(
          deadline_info,
          node_objects.api->StateMinerProvingDeadline(params.miner, {}));
      const auto min_exp = start_epoch + params.min_blocks_duration;
      const auto end_epoch =
          min_exp + deadline_info.wpost_proving_period
          - (min_exp % deadline_info.wpost_proving_period)
          + (deadline_info.period_start % deadline_info.wpost_proving_period)
          - 1;

      OUTCOME_TRY(network_version, node_objects.api->StateNetworkVersion({}));
      OUTCOME_TRY(seal_proof_type,
                  getPreferredSealProofTypeFromWindowPoStType(
                      network_version, miner_info.window_post_proof_type));

      return node_objects.storage_market_client->proposeStorageDeal(
          params.wallet,
          provider_info,
          params.data,
          start_epoch,
          end_epoch,
          params.epoch_price,
          params.provider_collateral,
          seal_proof_type,
          params.fast_retrieval);
    };
    node_objects.api->ClientListImports = [&]() {
      return node_objects.storage_market_import_manager->list();
    };

    auto rpc{api::makeRpc(*node_objects.api)};
    auto routes{std::make_shared<api::Routes>()};
    api::serve(
        rpc, routes, *node_objects.io_context, "127.0.0.1", config.api_port);
    api::rpc::saveInfo(config.repo_path, config.api_port, "stub");
    log()->info("API started at ws://127.0.0.1:{}", config.api_port);
  }

  void main(node::Config &config) {
    vm::actor::cgo::configParams();

    if (config.log_level <= spdlog::level::debug) {
      suppressVerboseLoggers();
    }

    auto obj_res = node::createNodeObjects(config);
    if (!obj_res) {
      log()->error("Cannot initialize node: {}", obj_res.error().message());
      exit(EXIT_FAILURE);
    }
    auto &o = obj_res.value();

    o.io_context->post([&] {
      for (auto &host : config.drand_servers) {
        drand::http::getInfo(*o.io_context, host, [&, host](auto _info) {
          if (_info) {
            auto &info{_info.value()};
            auto expect{[&](const auto &label,
                            const auto &actual,
                            const auto &expected) {
              if (actual != expected) {
                log()->error("drand config {}: {} expected {} got {}",
                             host,
                             label,
                             expected,
                             actual);
                return false;
              }
              return true;
            }};
            if (expect("key", info.key, *config.drand_bls_pubkey)
                && expect(
                    "genesis", info.genesis.count(), *config.drand_genesis)
                && expect(
                    "period", info.period.count(), *config.drand_period)) {
              return;
            }
          } else {
            log()->warn("drand config {}: {:#}", host, _info.error());
          }
          exit(EXIT_FAILURE);
        });
      }
    });

    log()->info("Starting components");

    auto events = o.events;

    node::PubsubWorkaround pubsub2{o.io_context,
                                   config.bootstrap_list,
                                   config.gossip_config,
                                   *config.network_name};

    auto conn = events->subscribeCurrentHead(
        [&](const sync::events::CurrentHead &head) {
          log()->info(
              "\n============================ {} ============================",
              head.tipset->height());
        });

    if (auto r = o.host->listen(config.p2pListenAddress()); !r) {
      log()->error("Cannot listen to {}: {}",
                   config.p2pListenAddress().getStringAddress(),
                   r.error().message());
      exit(EXIT_FAILURE);
    }

    o.host->start();
    auto listen_addresses = o.host->getAddresses();
    if (listen_addresses.empty()) {
      log()->error("Cannot listen to {}",
                   config.p2pListenAddress().getStringAddress());
      exit(EXIT_FAILURE);
    }

    log()->info(
        "Node started at {}, host PeerId {}",
        nonZeroAddr(listen_addresses, &config.localIp())->getStringAddress(),
        o.host->getId().toBase58());

    for (const auto &pi : config.bootstrap_list) {
      o.host->connect(pi);
    }

    if (config.use_pubsub_workaround) {
      auto workaround_peer_res = pubsub2.start(0);
      if (!workaround_peer_res) {
        log()->warn("cannot start pubsub workaround, {}",
                    workaround_peer_res.error().message());
      } else {
        o.gossip->addBootstrapPeer(
            workaround_peer_res.value().id,
            *nonZeroAddr(workaround_peer_res.value().addresses));
        log()->info("Started PubsubWorkaround at {}",
                    peerInfoToPrettyString(workaround_peer_res.value()));
      }
    }

    startApi(config, o);

    o.identify->start(events);
    o.say_hello->start(config.genesis_cid.value(), events);
    o.receive_hello->start();
    o.gossip->start();
    o.pubsub_gate->start(*config.network_name, events);
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
      exit(EXIT_FAILURE);
    }

    // gracefully shutdown on signal
    boost::asio::signal_set signals(*o.io_context, SIGINT, SIGTERM);
    signals.async_wait(
        [&](const boost::system::error_code &, int) { o.io_context->stop(); });

    // run event loop
    o.io_context->run();
    log()->info("Node stopped");
  }
}  // namespace fc

int main(int argc, char *argv[]) {
  fc::libp2pSoralog();

  try {
    auto config{fc::node::Config::read(argc, argv)};
    fc::main(config);
  } catch (const std::exception &e) {
    std::cerr << "UNEXPECTED EXCEPTION, " << e.what() << "\n";
  } catch (...) {
    std::cerr << "UNEXPECTED EXCEPTION\n";
  }
}
