/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "builder.hpp"

#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>

#include <leveldb/cache.h>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/identify/identify_delta.hpp>
#include <libp2p/protocol/identify/identify_push.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/content_routing_table_impl.hpp>
#include <libp2p/protocol/kademlia/impl/kademlia_impl.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp>
#include <libp2p/protocol/kademlia/impl/storage_backend_default.hpp>
#include <libp2p/protocol/kademlia/impl/storage_impl.hpp>
#include <libp2p/protocol/kademlia/impl/validator_default.hpp>

#include "api/full_node/make.hpp"
#include "api/impl/paych_get.hpp"
#include "api/impl/paych_voucher.hpp"
#include "api/setup_common.hpp"
#include "api/wallet/ledger_wallet.hpp"
#include "api/wallet/local_wallet.hpp"
#include "blockchain/block_validator/validator.hpp"
#include "blockchain/impl/weight_calculator_impl.hpp"
#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/ipld_version.hpp"
#include "clock/impl/chain_epoch_clock_impl.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "codec/json/json.hpp"
#include "common/api_secret.hpp"
#include "common/error_text.hpp"
#include "common/libp2p/timer_loop.hpp"
#include "common/peer_key.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "drand/impl/beaconizer.hpp"
#include "markets/discovery/impl/discovery_impl.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/retrieval/client/impl/retrieval_client_impl.hpp"
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "markets/storage/client/impl/storage_market_client_impl.hpp"
#include "markets/storage/types.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "primitives/tipset/chain.hpp"
#include "primitives/tipset/file.hpp"
#include "storage/car/car.hpp"
#include "storage/car/cids_index/util.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/compacter/util.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/keystore/impl/filesystem/filesystem_keystore.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/actor/builtin/states/init/init_actor_state.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/cached_interpreter.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::node {
  using markets::discovery::DiscoveryImpl;
  using markets::pieceio::PieceIOImpl;
  using markets::retrieval::client::RetrievalClientImpl;
  using markets::storage::kStorageMarketImportDir;
  using markets::storage::client::StorageMarketClientImpl;
  using primitives::tipset::TipsetCPtr;
  using storage::keystore::FileSystemKeyStore;
  using vm::actor::builtin::states::InitActorStatePtr;
  using vm::interpreter::InterpreterCache;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger;
    }

    outcome::result<void> initNetworkName(
        const primitives::tipset::Tipset &genesis_tipset,
        IpldPtr ipld,
        Config &config) {
      ipld = withVersion(ipld, 0);
      OUTCOME_TRY(init_actor,
                  vm::state::StateTreeImpl(
                      ipld, genesis_tipset.blks[0].parent_state_root)
                      .get(vm::actor::kInitAddress));
      OUTCOME_TRY(init_state,
                  getCbor<InitActorStatePtr>(ipld, init_actor.head));
      config.network_name = init_state->network_name;
      return outcome::success();
    }

    std::shared_ptr<libp2p::protocol::kademlia::KademliaImpl> createKademlia(
        Config &config,
        const NodeObjects &o,
        std::shared_ptr<libp2p::peer::IdentityManager> id_manager,
        std::shared_ptr<libp2p::event::Bus> bus) {
      config.kademlia_config.protocolId =
          std::string("/fil/kad/") + *config.network_name + "/kad/1.0.0";

      config.kademlia_config.randomWalk.enabled = false;

      std::shared_ptr<libp2p::protocol::kademlia::Storage> kad_storage =
          std::make_shared<libp2p::protocol::kademlia::StorageImpl>(
              config.kademlia_config,
              std::make_shared<
                  libp2p::protocol::kademlia::StorageBackendDefault>(),
              o.scheduler);

      std::shared_ptr<libp2p::protocol::kademlia::ContentRoutingTable>
          content_routing_table = std::make_shared<
              libp2p::protocol::kademlia::ContentRoutingTableImpl>(
              config.kademlia_config, *o.scheduler, bus);

      std::shared_ptr<libp2p::protocol::kademlia::PeerRoutingTable>
          peer_routing_table = std::make_shared<
              libp2p::protocol::kademlia::PeerRoutingTableImpl>(
              config.kademlia_config, id_manager, bus);

      std::shared_ptr<libp2p::protocol::kademlia::Validator> validator =
          std::make_shared<libp2p::protocol::kademlia::ValidatorDefault>();

      std::shared_ptr<libp2p::crypto::random::RandomGenerator>
          random_generator =
              std::make_shared<libp2p::crypto::random::BoostRandomGenerator>();

      return std::make_shared<libp2p::protocol::kademlia::KademliaImpl>(
          config.kademlia_config,
          o.host,
          std::move(kad_storage),
          std::move(content_routing_table),
          std::move(peer_routing_table),
          std::move(validator),
          o.scheduler,
          std::move(bus),
          std::move(random_generator));
    }

  }  // namespace

  auto loadSnapshot(Config &config, NodeObjects &o) {
    std::vector<CID> snapshot_cids;
    auto snapshot_key{
        std::make_shared<storage::OneKey>("snapshot", o.kv_store)};
    if (snapshot_key->has()) {
      snapshot_key->getCbor(snapshot_cids);
      if (!config.snapshot) {
        log()->warn(
            "snapshot was used before, but snapshot argument is missing");
      }
    }
    if (config.snapshot) {
      auto roots{storage::car::readHeader(*config.snapshot).value()};
      if (!snapshot_cids.empty() && snapshot_cids != roots) {
        log()->error("another snapshot already used");
        exit(EXIT_FAILURE);
      }
      // TODO(turuslan): max memory
      o.ipld_cids = *storage::cids_index::loadOrCreateWithProgress(
          *config.snapshot, false, boost::none, o.ipld, log());
      o.ipld = o.ipld_cids;
      if (snapshot_cids.empty()) {
        snapshot_cids = roots;
        log()->info("snapshot is ready to use");
        snapshot_key->setCbor(snapshot_cids);
      }
    }
    return snapshot_cids;
  }

  void loadChain(Config &config,
                 NodeObjects &o,
                 std::vector<CID> snapshot_cids) {
    log()->info("loading chain");
    const TipsetKey genesis_tsk{{*asBlake(*config.genesis_cid)}};
    const auto tsk{snapshot_cids.empty() ? genesis_tsk
                                         : *TipsetKey::make(snapshot_cids)};
    bool updated{};
    // TODO: refactor o.ipld to CbIpld
    // estimated const
    o.ts_main = primitives::tipset::chain::file::loadOrCreate(
        &updated, config.join("ts-chain"), o.compacter, tsk.cids(), 20, 1000);
    if (!o.ts_main) {
      log()->error("chain load error");
      exit(EXIT_FAILURE);
    }
    if (updated) {
      auto it{std::prev(o.ts_main->chain.end())};
      while (true) {
        auto ts{o.ts_load->lazyLoad(it->second).value()};
        if (auto _has{o.ipld->contains(ts->getParentStateRoot())};
            _has && _has.value()) {
          o.env_context.interpreter_cache->set(
              ts->getParents(),
              {
                  ts->getParentStateRoot(),
                  ts->getParentMessageReceipts(),
                  ts->getParentWeight(),
              });
          if (it != o.ts_main->chain.begin()) {
            --it;
            continue;
          }
        }
        break;
      }
    }

    while (true) {
      const auto it{std::prev(o.ts_main->chain.end())};
      if (it == o.ts_main->chain.begin()
          || o.env_context.interpreter_cache->tryGet(it->second.key)) {
        break;
      }
      log()->warn("missing state at {}, reverting", it->first);
      o.ts_main->updater->revert();
      o.ts_main->chain.erase(it);
    }

    log()->info("chain loaded");
    assert(o.ts_main->bottom().second.key == genesis_tsk);
  }

  auto writableIpld(Config &config, NodeObjects &o) {
    auto car_path{config.join("cids_index.car")};
    // TODO(turuslan): max memory
    // estimated, 1gb
    auto ipld = *storage::cids_index::loadOrCreateWithProgress(
        car_path, true, 1 << 30, o.ipld, log());
    // estimated
    ipld->flush_on = 200000;
    ipld->car_flush_on = 100;
    o.ipld_flush_thread = std::make_shared<IoThread>();
    ipld->io = o.ipld_flush_thread->io;
    return ipld;
  }

  outcome::result<KeyInfo> readPrivateKeyFromFile(const std::string &path) {
    std::ifstream ifs(path);
    std::string hex_string(std::istreambuf_iterator<char>{ifs}, {});
    OUTCOME_TRY(blob, common::unhex(hex_string));
    OUTCOME_TRY(json, codec::json::parse(blob));
    OUTCOME_TRY(key_info, codec::json::decode<KeyInfo>(json));
    return std::move(key_info);
  }

  /**
   * Creates and initializes message pool and sets timer for republishing.
   * @param node_objects
   * @return
   */
  void createMessagePool(const Config &config, NodeObjects &o) {
    o.mpool = storage::mpool::MessagePool::create(o.env_context,
                                                  o.ts_main,
                                                  config.mpool_bls_cache_size,
                                                  o.chain_store,
                                                  o.pubsub_gate);
    // Republish pending messages
    // Delay from lotus
    // https://github.com/filecoin-project/lotus/blob/d9100981ada8b3186d906a4f4140b83a819d2299/chain/messagepool/messagepool.go#L58
    const auto republishTimeout{
        std::chrono::seconds(10 * kBlockDelaySecs + kPropagationDelaySecs)};
    timerLoop(o.scheduler, republishTimeout, [mpool{o.mpool}] {
      const auto res = mpool->republishPendingMessages();
      if (!res) {
        log()->error("Mpool republish error: {:#}", res.error());
      }
    });
    // batch message publishing with delay kRepublishBatchDelay
    timerLoop(o.scheduler,
              storage::mpool::kRepublishBatchDelay,
              [mpool{o.mpool}] { mpool->publishFromQueue(); });
  }

  /**
   * Creates and intialises Storage Market Client
   * @param o - Node objecs
   */
  outcome::result<void> createStorageMarketClient(NodeObjects &node_objects) {
    node_objects.storage_market_import_manager =
        std::make_shared<ImportManager>(
            std::make_shared<storage::MapPrefix>("storage_market_imports/",
                                                 node_objects.kv_store),
            kStorageMarketImportDir);
    node_objects.chain_events = std::make_shared<ChainEventsImpl>(
        node_objects.api, ChainEventsImpl::IsDealPrecommited{});
    node_objects.market_discovery =
        std::make_shared<DiscoveryImpl>(std::make_shared<storage::MapPrefix>(
            "discovery/", node_objects.kv_store)),
    node_objects.storage_market_client =
        std::make_shared<StorageMarketClientImpl>(
            node_objects.host,
            node_objects.io_context,
            node_objects.storage_market_import_manager,
            node_objects.datatransfer,
            node_objects.market_discovery,
            node_objects.api,
            node_objects.chain_events,
            std::make_shared<PieceIOImpl>("/tmp/fuhon/piece_io"));
    // timer is set to 5000 ms
    timerLoop(node_objects.scheduler,
              std::chrono::milliseconds(5000),
              [client{node_objects.storage_market_client}] {
                client->pollWaiting();
              });
    return node_objects.storage_market_client->init();
  }

  outcome::result<void> createRetrievalMarketClient(NodeObjects &node_objects) {
    node_objects.retrieval_market_client =
        std::make_shared<RetrievalClientImpl>(node_objects.host,
                                              node_objects.datatransfer,
                                              node_objects.api,
                                              node_objects.markets_ipld);
    return outcome::success();
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<NodeObjects> createNodeObjects(Config &config) {
    NodeObjects o;

    log()->debug("Creating storage...");

    auto leveldb_res = storage::LevelDB::create(config.join("leveldb"));
    if (!leveldb_res) {
      return Error::kStorageInitError;
    }
    o.kv_store = std::move(leveldb_res.value());

    o.ipld_leveldb_kv =
        storage::LevelDB::create(config.join("ipld_leveldb")).value();
    o.ipld_leveldb =
        std::make_shared<storage::ipfs::LeveldbDatastore>(o.ipld_leveldb_kv);
    o.ipld = *storage::cids_index::loadOrCreateWithProgress(
        config.genesisCar(), false, boost::none, o.ipld, log());
    auto snapshot_cids{loadSnapshot(config, o)};

    auto ts_mutex{std::make_shared<std::shared_mutex>()};
    o.compacter = storage::compacter::make(config.join("compacter"),
                                           o.kv_store,
                                           writableIpld(config, o),
                                           ts_mutex);
    o.ipld = std::make_shared<CbAsAnyIpld>(o.compacter);

    // estimated, 80gb
    o.compacter->compact_on_car = uint64_t{80} << 30;
    o.compacter->epochs_full_state = 30;
    o.compacter->epochs_lookback_state = 2400;
    o.compacter->epochs_messages = 60;

    o.ts_load_ipld = std::make_shared<primitives::tipset::TsLoadIpld>(o.ipld);
    o.compacter->ts_load = o.ts_load_ipld;
    o.ts_load = std::make_shared<primitives::tipset::TsLoadCache>(
        o.ts_load_ipld, 8 << 10);

    auto genesis_cids{storage::car::readHeader(config.genesisCar()).value()};
    assert(genesis_cids.size() == 1);
    config.genesis_cid = genesis_cids[0];

    const auto genesis{o.ts_load->load(*TipsetKey::make(genesis_cids)).value()};
    const clock::UnixTime genesis_timestamp{genesis->blks[0].timestamp};

    log()->info("Genesis: {}, timestamp {}",
                config.genesis_cid.value().toString().value(),
                clock::unixTimeToString(genesis_timestamp));

    const drand::ChainInfo drand_chain_info{
        .key = *config.drand_bls_pubkey,
        .genesis = std::chrono::seconds(*config.drand_genesis),
        .period = std::chrono::seconds(*config.drand_period),
    };

    const auto drand_schedule{std::make_shared<drand::DrandScheduleImpl>(
        drand_chain_info,
        genesis_timestamp,
        std::chrono::seconds(kBlockDelaySecs))};

    o.env_context.ts_branches_mutex = ts_mutex;
    o.env_context.ipld = o.ipld;
    o.env_context.invoker = std::make_shared<vm::actor::InvokerImpl>();
    o.env_context.randomness = std::make_shared<vm::runtime::TipsetRandomness>(
        o.ts_load, o.env_context.ts_branches_mutex, drand_schedule);
    o.env_context.ts_load = o.ts_load;
    o.env_context.interpreter_cache =
        std::make_shared<vm::interpreter::InterpreterCache>(
            std::make_shared<storage::MapPrefix>("vm/", o.kv_store),
            std::make_shared<AnyAsCbIpld>(o.ipld));
    OUTCOME_TRYA(o.env_context.circulating,
                 vm::Circulating::make(o.ipld, *config.genesis_cid));

    auto block_validator{
        std::make_shared<blockchain::block_validator::BlockValidator>(
            std::make_shared<storage::MapPrefix>("block_validator/",
                                                 o.kv_store),
            o.env_context)};

    auto weight_calculator =
        std::make_shared<blockchain::weight::WeightCalculatorImpl>(o.ipld);

    o.interpreter = std::make_shared<vm::interpreter::InterpreterImpl>(
        o.env_context, block_validator, weight_calculator);
    o.vm_interpreter = std::make_shared<vm::interpreter::CachedInterpreter>(
        o.interpreter, o.env_context.interpreter_cache);
    o.compacter->interpreter->interpreter = o.vm_interpreter;
    o.vm_interpreter = o.compacter->interpreter;

    loadChain(config, o, snapshot_cids);
    o.ts_branches = std::make_shared<TsBranches>();
    o.ts_branches->insert(o.ts_main);

    o.compacter->interpreter_cache = o.env_context.interpreter_cache;
    o.compacter->ts_branches = o.ts_branches;
    o.compacter->ts_main = o.ts_main;
    o.compacter->open();

    OUTCOME_TRY(initNetworkName(*genesis, o.ipld, config));
    log()->info("Network name: {}", *config.network_name);

    o.utc_clock = std::make_shared<clock::UTCClockImpl>();

    o.chain_epoch_clock =
        std::make_shared<clock::ChainEpochClockImpl>(genesis_timestamp);

    log()->debug("Creating host...");

    OUTCOME_TRY(keypair, loadPeerKey(config.join("peer_ed25519.key")));

    auto injector = libp2p::injector::makeHostInjector<
        boost::di::extension::shared_config>(
        libp2p::injector::useKeyPair(keypair),
        boost::di::bind<clock::UTCClock>.template to<clock::UTCClockImpl>());

    o.io_context = injector.create<std::shared_ptr<boost::asio::io_context>>();
    o.scheduler = injector.create<std::shared_ptr<Scheduler>>();

    timerLoop(o.scheduler, std::chrono::minutes{1}, [ipld{o.compacter}] {
      ipld->carFlush();
    });

    o.events = std::make_shared<sync::events::Events>(o.io_context);

    o.host = injector.create<std::shared_ptr<libp2p::Host>>();

    log()->debug("Creating protocols...");

    auto identify_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::Identify>>();
    auto identify_push_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::IdentifyPush>>();
    auto identify_delta_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::IdentifyDelta>>();

    o.identify =
        std::make_shared<sync::Identify>(o.host,
                                         std::move(identify_protocol),
                                         std::move(identify_push_protocol),
                                         std::move(identify_delta_protocol));

    o.say_hello =
        std::make_shared<sync::SayHello>(o.host, o.scheduler, o.utc_clock);

    o.receive_hello = std::make_shared<sync::ReceiveHello>(
        o.host, o.utc_clock, *config.genesis_cid, o.events);

    o.gossip = libp2p::protocol::gossip::create(
        o.scheduler,
        o.host,
        injector.create<std::shared_ptr<libp2p::peer::IdentityManager>>(),
        injector.create<std::shared_ptr<libp2p::crypto::CryptoProvider>>(),
        injector.create<
            std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>>(),
        config.gossip_config);

    using libp2p::protocol::gossip::ByteArray;
    o.gossip->setMessageIdFn(
        [](const ByteArray &from, const ByteArray &seq, const ByteArray &data) {
          auto h = crypto::blake2b::blake2b_256(data);
          return ByteArray(h.data(), h.data() + h.size());
        });

    o.pubsub_gate = std::make_shared<sync::PubSubGate>(o.gossip);

    auto id_manager =
        injector.create<std::shared_ptr<libp2p::peer::IdentityManager>>();

    auto bus = injector.create<std::shared_ptr<libp2p::event::Bus>>();

    auto kademlia =
        createKademlia(config, o, std::move(id_manager), std::move(bus));

    o.peer_discovery = std::make_shared<sync::PeerDiscovery>(
        o.host, o.scheduler, std::move(kademlia));

    o.graphsync = std::make_shared<storage::ipfs::graphsync::GraphsyncImpl>(
        o.host, o.scheduler);

    o.graphsync_server =
        std::make_shared<sync::GraphsyncServer>(o.graphsync, o.ipld);

    log()->debug("Creating chain loaders...");

    o.blocksync_server = std::make_shared<fc::sync::blocksync::BlocksyncServer>(
        o.host, o.ts_load_ipld, o.ipld);

    log()->debug("Creating chain store...");

    auto bls_provider = std::make_shared<crypto::bls::BlsProviderImpl>();

    auto secp_provider =
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>();

    auto head{
        o.ts_load->lazyLoad(std::prev(o.ts_main->chain.end())->second).value()};
    if (!o.env_context.interpreter_cache->tryGet(head->key)) {
      log()->info("interpret head {}", head->height());
      o.vm_interpreter->interpret(o.ts_main, head).value();
    }
    auto head_weight{
        o.env_context.interpreter_cache->get(head->key).value().weight};
    o.chain_store = std::make_shared<sync::ChainStoreImpl>(
        o.ipld, o.ts_load, o.compacter->put_block_header, head, head_weight);

    o.sync_job =
        std::make_shared<sync::SyncJob>(o.host,
                                        o.io_context,
                                        o.chain_store,
                                        o.scheduler,
                                        o.vm_interpreter,
                                        o.env_context.interpreter_cache,
                                        o.env_context.ts_branches_mutex,
                                        o.ts_branches,
                                        o.ts_main,
                                        o.ts_load,
                                        o.compacter->put_block_header,
                                        o.ipld);

    log()->debug("Creating API...");

    createMessagePool(config, o);

    auto msg_waiter = storage::blockchain::MsgWaiter::create(
        o.ts_load, o.ipld, o.io_context, o.chain_store);

    o.key_store = std::make_shared<storage::keystore::FileSystemKeyStore>(
        (config.repo_path / "keystore").string(), bls_provider, secp_provider);
    o.wallet_default_address =
        std::make_shared<OneKey>("wallet_default_address", o.kv_store);
    // If default key is set, import to keystore and save default key address.
    // Default key must be a BLS key.
    if (config.wallet_default_key_path) {
      const auto maybe_default_key =
          readPrivateKeyFromFile(*config.wallet_default_key_path);
      if (maybe_default_key.has_error()) {
        log()->error("Cannot read default key from {}",
                     *config.wallet_default_key_path);
        return maybe_default_key.error();
      }
      auto key_info{maybe_default_key.value()};
      OUTCOME_TRY(private_key, key_info.getPrivateKey());
      OUTCOME_TRY(address, o.key_store->put(key_info.type, private_key));
      o.wallet_default_address->setCbor(address);
      log()->info("Set default wallet address {}", address);
    } else {
      if (o.wallet_default_address->has()) {
        log()->info("Load default wallet address {}",
                    o.wallet_default_address->getCbor<Address>());
      }
    }

    if (config.drand_servers.empty()) {
      config.drand_servers.emplace_back("https://127.0.0.1:8080");
    }

    auto beaconizer =
        std::make_shared<drand::BeaconizerImpl>(o.io_context,
                                                o.utc_clock,
                                                o.scheduler,
                                                drand_chain_info,
                                                config.drand_servers,
                                                config.beaconizer_cache_size);

    o.markets_ipld = o.ipld_leveldb;
    o.api = std::make_shared<api::FullNodeApi>();
    o.datatransfer = DataTransfer::make(o.host, o.graphsync);
    OUTCOME_TRY(createStorageMarketClient(o));
    OUTCOME_TRY(createRetrievalMarketClient(o));

    OUTCOME_TRY(api_secret, loadApiSecret(config.join("jwt_secret")));

    auto tipsetContext =
        [=](const TipsetKey &tipset_key,
            bool interpret) -> outcome::result<api::TipsetContext> {
      TipsetCPtr tipset;
      if (tipset_key.cids().empty()) {
        tipset = o.chain_store->heaviestTipset();
      } else {
        OUTCOME_TRYA(tipset, o.env_context.ts_load->load(tipset_key));
      }
      auto ipld{withVersion(o.env_context.ipld, tipset->height())};
      api::TipsetContext context{
          tipset, {ipld, tipset->getParentStateRoot()}, {}};
      if (interpret) {
        OUTCOME_TRY(result, o.env_context.interpreter_cache->get(tipset->key));
        context.state_tree = {ipld, result.state_root};
        context.interpreted = result;
      }
      return context;
    };

    o.api = api::makeImpl(o.api,
                          o.chain_store,
                          o.markets_ipld,
                          *config.network_name,
                          weight_calculator,
                          o.env_context,
                          o.ts_main,
                          o.mpool,
                          msg_waiter,
                          beaconizer,
                          drand_schedule,
                          o.pubsub_gate,
                          o.key_store,
                          o.market_discovery,
                          o.retrieval_market_client,
                          tipsetContext);
    api::fillPaychGet(
        o.api,
        std::make_shared<paych_maker::PaychMaker>(
            o.api,
            std::make_shared<storage::MapPrefix>("paych_maker/", o.kv_store)));

    api::fillPaychVoucher(o.api,
                          std::make_shared<paych_vouchers::PaychVouchers>(
                              o.ipld,
                              o.api,
                              std::make_shared<storage::MapPrefix>(
                                  "paych_vouchers/", o.kv_store)));

    api::fillAuthApi(o.api, api_secret, api::kNodeApiLogger);

    api::LocalWallet::fillLocalWalletApi(
        o.api, o.key_store, tipsetContext, o.wallet_default_address);
    api::LedgerWallet::fillLedgerWalletApi(
        o.api, std::make_shared<storage::MapPrefix>("ledger/", o.kv_store));

    o.chain_events->init().value();

    return o;
  }
}  // namespace fc::node

OUTCOME_CPP_DEFINE_CATEGORY(fc::node, Error, e) {
  using E = fc::node::Error;

  switch (e) {
    case E::kStorageInitError:
      return "cannot initialize storage";
    case E::kCarOpenFileError:
      return "cannot open initial car file";
    case E::kCarFileAboveLimit:
      return "car file size above limit";
    case E::kNoGenesisBlock:
      return "no genesis block";
    case E::kGenesisMismatch:
      return "genesis mismatch";
    default:
      break;
  }
  return "node::Error: unknown error";
}
