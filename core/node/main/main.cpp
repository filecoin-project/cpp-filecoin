/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <spdlog/sinks/basic_file_sink.h>
#include <sys/resource.h>
#include <iostream>

#include "api/full_node/make.hpp"
#include "api/full_node/node_api_v1_wrapper.hpp"
#include "api/network/setup_net.hpp"
#include "api/rpc/info.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "common/api_secret.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "common/libp2p/soralog.hpp"
#include "common/local_ip.hpp"
#include "common/logger.hpp"
#include "common/prometheus/rpc.hpp"
#include "drand/impl/http.hpp"
#include "markets/storage/types.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/main/builder.hpp"
#include "node/main/metrics.hpp"
#include "node/node_version.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/pubsub_workaround.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "vm/actor/cgo/actors.hpp"

void setFdLimitMax() {
  rlimit r{};
  if (getrlimit(RLIMIT_NOFILE, &r) != 0) {
    return spdlog::error("getrlimit(RLIMIT_NOFILE), errno={}", errno);
  }
  if (r.rlim_max == RLIM_INFINITY) {
    return;
  }
  r.rlim_cur = r.rlim_max;
  if (setrlimit(RLIMIT_NOFILE, &r) != 0) {
    return spdlog::error(
        "setrlimit(RLIMIT_NOFILE, {}), errno={}", r.rlim_cur, errno);
  }
}

namespace fc {
  using api::Import;
  using api::ImportRes;
  using api::makeFullNodeApiV1Wrapper;
  using api::StorageMarketDealInfo;
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;
  using markets::storage::StorageProviderInfo;
  using node::Metrics;
  using node::NodeObjects;
  using primitives::jwt::kAllPermission;
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

  void startApi(const node::Config &config,
                NodeObjects &node_objects,
                const Metrics &metrics) {
    auto &o{node_objects};

    const PeerInfo api_peer_info{
        o.host->getId(),
        nonZeroAddrs(o.host->getAddresses(), &common::localIp()),
    };
    api::fillNetApi(o.api, api_peer_info, o.host, api::kNodeApiLogger);

    // Market Client API
    node_objects.api->ClientImport =
        [&](auto &file_ref) -> outcome::result<ImportRes> {
      OUTCOME_TRY(root,
                  node_objects.storage_market_import_manager->import(
                      file_ref.path, file_ref.is_car));
      // storage id set to 0
      return ImportRes{root, 0};
    };

    node_objects.api->ClientListDeals = [&node_objects]()
        -> outcome::result<std::vector<StorageMarketDealInfo>> {
      std::vector<StorageMarketDealInfo> result;
      OUTCOME_TRY(local_deals,
                  node_objects.storage_market_client->listLocalDeals());
      result.reserve(local_deals.size());
      for (const auto &deal : local_deals) {
        result.emplace_back(StorageMarketDealInfo{
            deal.proposal_cid,
            deal.state,
            deal.message,
            deal.client_deal_proposal.proposal.provider,
            deal.data_ref,
            deal.client_deal_proposal.proposal.piece_cid,
            deal.client_deal_proposal.proposal.piece_size.unpadded(),
            deal.client_deal_proposal.proposal.storage_price_per_epoch,
            deal.client_deal_proposal.proposal.duration(),
            deal.deal_id,
            // TODO (a.chernyshov) creation time - actually not used
            {},
            deal.client_deal_proposal.proposal.verified,
            // TODO (a.chernyshov) actual ChannelId
            {node_objects.host->getId(), deal.miner.id, 0},
            // TODO (a.chernyshov) actual data transfer
            {0, 0, deal.proposal_cid, true, true, "", "", deal.miner.id, 0}});
      }
      return result;
    };

 
    node_objects.api->ClientGetDealInfo =
        [&node_objects](auto &cid) -> outcome::result<StorageMarketDealInfo> {
      OUTCOME_TRY(deal, node_objects.storage_market_client->getLocalDeal(cid));
      return StorageMarketDealInfo{
          .proposal_cid = deal.proposal_cid,
          .state = deal.state,
          .message = deal.message,
          .provider = deal.client_deal_proposal.proposal.provider,
          .data_ref = deal.data_ref,
          .piece_cid = deal.client_deal_proposal.proposal.piece_cid,
          .size = deal.client_deal_proposal.proposal.piece_size.unpadded(),
          .price_per_epoch =
              deal.client_deal_proposal.proposal.storage_price_per_epoch,
          .duration = deal.client_deal_proposal.proposal.duration(),
          .deal_id = deal.deal_id,
          .verified = deal.client_deal_proposal.proposal.verified,
      };  // TODO(@Elestrias): [FIL-614] Creation time
    };

    node_objects.api->ClientListRetrievals = [&node_objects]()
        -> outcome::result<std::vector<api::RetrievalDeal>> {
      return node_objects.retrieval_market_client->getRetrievals();
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
      const StorageProviderInfo provider_info{
          .address = params.miner,
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
          params.verified_deal,
          params.fast_retrieval);
    };
    node_objects.api->ClientListImports =
        [&]() -> outcome::result<std::vector<Import>> {
      return node_objects.storage_market_import_manager->list();
    };
    node_objects.api_v1 = makeFullNodeApiV1Wrapper();
    auto rpc_v1{api::makeRpc(
        *node_objects.api,
        std::bind(node_objects.api->AuthVerify, std::placeholders::_1))};
    wrapRpc(rpc_v1, *node_objects.api_v1);

    auto rpc{api::makeRpc(
        *node_objects.api,
        std::bind(node_objects.api->AuthVerify, std::placeholders::_1))};

    metricApiTime(*rpc_v1);
    metricApiTime(*rpc);

    std::map<std::string, std::shared_ptr<api::Rpc>> rpcs;
    rpcs.emplace("/rpc/v0", rpc_v1);
    rpcs.emplace("/rpc/v1", rpc);

    auto routes{std::make_shared<api::Routes>()};

    auto text_route{[](auto f) -> api::RouteHandler {
      return [f{std::move(f)}](auto &, auto &cb) {
        api::http::response<api::http::string_body> res;
        res.body() = f();
        cb(api::WrapperResponse{std::move(res)});
      };
    }};
    routes->emplace("/health",
                    text_route([] { return "{\"status\":\"UP\"}"; }));
    routes->emplace("/metrics",
                    text_route([&] { return metrics.prometheus(); }));

    api::serve(
        rpcs, routes, *node_objects.io_context, config.api_ip, config.api_port);
    auto api_secret = loadApiSecret(config.join("jwt_secret")).value();
    auto token = generateAuthToken(api_secret, kAllPermission).value();
    api::rpc::saveInfo(config.repo_path, config.api_port, token);
    log()->info("API started at ws://127.0.0.1:{}", config.api_port);
  }

  void main(node::Config &config) {
    log()->debug("Starting ", node::kNodeVersion);

    const auto start_time{Metrics::Clock::now()};

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

    auto gs_sub{o.graphsync->subscribe([&](auto &, auto &data) {
      o.markets_ipld->set(data.cid, BytesIn{data.content}).value();
    })};

    auto mpool_gossip{o.events->subscribeMessageFromPubSub([&](auto &e) {
      auto res{o.mpool->add(e.msg)};
      if (!res) {
        spdlog::error("MessagePool.subscribeMessageFromPubSub: {:#}",
                      res.error());
      }
    })};

    Metrics metrics{o, start_time};

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
        nonZeroAddr(listen_addresses, &common::localIp())->getStringAddress(),
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

    startApi(config, o, metrics);  // may throw

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
  setFdLimitMax();

  auto config{fc::node::Config::read(argc, argv)};
  fc::libp2pSoralog(config.join("libp2p.log"));

  using fc::common::file_sink;
  file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      config.join("fuhon.log"));
  spdlog::default_logger()->sinks().push_back(file_sink);

  fc::main(config);
}
