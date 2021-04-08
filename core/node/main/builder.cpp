/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "builder.hpp"

#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>

#include <leveldb/cache.h>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
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

#include "api/make.hpp"
#include "blockchain/block_validator/impl/block_validator_impl.hpp"
#include "blockchain/impl/weight_calculator_impl.hpp"
#include "clock/impl/chain_epoch_clock_impl.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "common/peer_key.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "drand/impl/beaconizer.hpp"
#include "markets/discovery/discovery.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "markets/storage/client/impl/storage_market_client_impl.hpp"
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "power/impl/power_table_impl.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/car/car.hpp"
#include "storage/car/cids_index/util.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::node {
  using markets::discovery::Discovery;
  using markets::pieceio::PieceIOImpl;
  using markets::storage::chain_events::ChainEventsImpl;
  using markets::storage::client::StorageMarketClientImpl;
  using markets::storage::client::import_manager::kImportsDir;
  using storage::InMemoryStorage;
  using storage::ipfs::InMemoryDatastore;

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger;
    }

    outcome::result<void> initNetworkName(
        const primitives::tipset::Tipset &genesis_tipset,
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        Config &config) {
      OUTCOME_TRY(init_actor,
                  vm::state::StateTreeImpl(
                      ipld, genesis_tipset.blks[0].parent_state_root)
                      .get(vm::actor::kInitAddress));
      OUTCOME_TRY(
          init_state,
          vm::actor::builtin::states::StateProvider{ipld}.getInitActorState(
              init_actor));
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

  auto ipldLeveldbOptions() {
    leveldb::Options options;
    options.create_if_missing = true;
    options.write_buffer_size = 128 << 20;
    options.block_cache = leveldb::NewLRUCache(128 << 20);
    options.block_size = 64 << 10;
    options.max_file_size = 128 << 20;
    return options;
  }

  auto loadSnapshot(Config &config, NodeObjects &o) {
    std::vector<CID> snapshot_cids;
    auto snapshot_key{
        std::make_shared<storage::OneKey>("snapshot", o.kv_store)};
    if (snapshot_key->has()) {
      snapshot_key->getCbor(snapshot_cids);
      if (!config.snapshot) {
        log()->warn(
            "snapshot was imported before, but snapshot argument is missing");
      }
    }
    if (config.snapshot) {
      auto roots{storage::car::readHeader(*config.snapshot).value()};
      if (!snapshot_cids.empty() && snapshot_cids != roots) {
        log()->error("another snapshot already imported");
        exit(EXIT_FAILURE);
      }
      // TODO(turuslan): max memory
      o.ipld_cids = storage::cids_index::loadOrCreateWithProgress(
                        *config.snapshot, false, boost::none, o.ipld, log())
                        .value();
      o.ipld = o.ipld_cids;
      if (snapshot_cids.empty()) {
        snapshot_cids = roots;
        log()->info("snapshot imported");
        snapshot_key->setCbor(snapshot_cids);
      }
    }
    return snapshot_cids;
  }

  void loadChain(Config &config,
                 NodeObjects &o,
                 std::vector<CID> snapshot_cids) {
    o.ts_main_kv = std::make_shared<storage::MapPrefix>("ts_main/", o.kv_store);
    log()->info("loading chain");
    o.ts_main = TsBranch::load(o.ts_main_kv);
    TipsetKey genesis_tsk{{*config.genesis_cid}};
    if (!o.ts_main) {
      auto tsk{genesis_tsk};
      if (!snapshot_cids.empty()) {
        log()->info("restoring chain from snapshot");
        tsk = snapshot_cids;
      }
      o.ts_main = TsBranch::create(o.ts_main_kv, tsk, o.ts_load_ipld).value();

      auto it{std::prev(o.ts_main->chain.end())};
      while (true) {
        auto ts{o.ts_load->loadw(it->second).value()};
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
    auto it{std::prev(o.ts_main->chain.end())};
    auto ts{o.ts_load->loadw(it->second).value()};
    if (!o.env_context.interpreter_cache->tryGet(ts->key)) {
      log()->info("interpret head {}", it->first);
      o.vm_interpreter->interpret(o.ts_main, ts).value();
    }

    log()->info("chain loaded");
    assert(o.ts_main->chain.begin()->second.key == genesis_tsk);
  }

  outcome::result<NodeObjects> createNodeObjects(Config &config) {
    NodeObjects o;

    log()->debug("Creating storage...");

    auto leveldb_res = storage::LevelDB::create(config.join("leveldb"));
    if (!leveldb_res) {
      return Error::kStorageInitError;
    }
    o.kv_store = std::move(leveldb_res.value());

    o.ipld_leveldb_kv = storage::LevelDB::create(config.join("ipld_leveldb"),
                                                 ipldLeveldbOptions())
                            .value();
    o.ipld_leveldb =
        std::make_shared<storage::ipfs::LeveldbDatastore>(o.ipld_leveldb_kv);
    o.ipld = o.ipld_leveldb;
    auto snapshot_cids{loadSnapshot(config, o)};

    o.ts_load_ipld = std::make_shared<primitives::tipset::TsLoadIpld>(o.ipld);
    o.ts_load = std::make_shared<primitives::tipset::TsLoadCache>(
        o.ts_load_ipld, 8 << 10);

    auto genesis_cids{
        storage::car::loadCar(*o.ipld, config.genesisCar()).value()};
    assert(genesis_cids.size() == 1);
    config.genesis_cid = genesis_cids[0];

    o.env_context.ts_branches_mutex = std::make_shared<std::shared_mutex>();
    o.env_context.ipld = o.ipld;
    o.env_context.invoker = std::make_shared<vm::actor::InvokerImpl>();
    o.env_context.randomness = std::make_shared<vm::runtime::TipsetRandomness>(
        o.ts_load, o.env_context.ts_branches_mutex);
    o.env_context.ts_load = o.ts_load;
    o.env_context.interpreter_cache =
        std::make_shared<vm::interpreter::InterpreterCache>(
            std::make_shared<storage::MapPrefix>("vm/", o.kv_store));
    OUTCOME_TRYA(o.env_context.circulating,
                 vm::Circulating::make(o.ipld, *config.genesis_cid));

    auto weight_calculator =
        std::make_shared<blockchain::weight::WeightCalculatorImpl>(o.ipld);

    o.interpreter = std::make_shared<vm::interpreter::InterpreterImpl>(
        o.env_context, weight_calculator);
    o.vm_interpreter = std::make_shared<vm::interpreter::CachedInterpreter>(
        o.interpreter, o.env_context.interpreter_cache);

    loadChain(config, o, snapshot_cids);
    o.ts_branches = std::make_shared<TsBranches>();
    o.ts_branches->insert(o.ts_main);

    OUTCOME_EXCEPT(genesis, o.ts_load->load(genesis_cids));
    OUTCOME_TRY(initNetworkName(*genesis, o.ipld, config));
    log()->info("Network name: {}", *config.network_name);

    auto genesis_timestamp = clock::UnixTime(genesis->blks[0].timestamp);

    log()->info("Genesis: {}, timestamp {}",
                config.genesis_cid.value().toString().value(),
                clock::unixTimeToString(genesis_timestamp));

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

    o.scheduler =
        injector.create<std::shared_ptr<libp2p::protocol::Scheduler>>();

    o.events = std::make_shared<sync::events::Events>(o.scheduler);

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
        o.scheduler, o.host, config.gossip_config);

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

    auto power_table = std::make_shared<power::PowerTableImpl>();

    auto bls_provider = std::make_shared<crypto::bls::BlsProviderImpl>();

    auto secp_provider =
        std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>();

    auto block_validator =
        std::make_shared<blockchain::block_validator::BlockValidatorImpl>(
            o.ipld,
            o.utc_clock,
            o.chain_epoch_clock,
            weight_calculator,
            power_table,
            bls_provider,
            secp_provider,
            o.env_context.interpreter_cache);

    auto head{
        o.ts_load->loadw(std::prev(o.ts_main->chain.end())->second).value()};
    auto head_weight{
        o.env_context.interpreter_cache->get(head->key).value().weight};
    o.chain_store = std::make_shared<sync::ChainStoreImpl>(
        o.ipld, o.ts_load, head, head_weight, std::move(block_validator));

    o.sync_job =
        std::make_shared<sync::SyncJob>(o.host,
                                        o.chain_store,
                                        o.scheduler,
                                        o.vm_interpreter,
                                        o.env_context.interpreter_cache,
                                        o.env_context.ts_branches_mutex,
                                        o.ts_branches,
                                        o.ts_main_kv,
                                        o.ts_main,
                                        o.ts_load,
                                        o.ipld);

    log()->debug("Creating API...");

    auto mpool = storage::mpool::MessagePool::create(
        o.env_context, o.ts_main, o.chain_store);

    auto msg_waiter = storage::blockchain::MsgWaiter::create(
        o.ts_load, o.ipld, o.chain_store);

    auto key_store = std::make_shared<storage::keystore::InMemoryKeyStore>(
        bls_provider, secp_provider);

    drand::ChainInfo drand_chain_info{
        .key = *config.drand_bls_pubkey,
        .genesis = std::chrono::seconds(*config.drand_genesis),
        .period = std::chrono::seconds(*config.drand_period),
    };

    if (config.drand_servers.empty()) {
      config.drand_servers.push_back("https://127.0.0.1:8080");
    }

    auto beaconizer =
        std::make_shared<drand::BeaconizerImpl>(o.io_context,
                                                o.utc_clock,
                                                o.scheduler,
                                                drand_chain_info,
                                                config.drand_servers,
                                                config.beaconizer_cache_size);

    auto drand_schedule = std::make_shared<drand::DrandScheduleImpl>(
        drand_chain_info,
        genesis_timestamp,
        std::chrono::seconds(kEpochDurationSeconds));

    o.datatransfer = DataTransfer::make(o.host, o.graphsync);

    o.storage_market_import_manager = std::make_shared<ImportManager>(
        std::make_shared<storage::MapPrefix>("storage_market_imports/",
                                             o.kv_store),
        kImportsDir);
    o.chain_events = std::make_shared<ChainEventsImpl>(o.api);
    o.storage_market_client = std::make_shared<StorageMarketClientImpl>(
        o.host,
        o.io_context,
        o.storage_market_import_manager,
        o.datatransfer,
        std::make_shared<Discovery>(
            std::make_shared<storage::MapPrefix>("discovery/", o.kv_store)),
        o.api,
        o.chain_events,
        std::make_shared<PieceIOImpl>("/tmp/fuhon/piece_io"));
    OUTCOME_TRY(o.storage_market_client->init());

    o.api = api::makeImpl(o.chain_store,
                          *config.network_name,
                          weight_calculator,
                          o.env_context,
                          o.ts_main,
                          mpool,
                          msg_waiter,
                          beaconizer,
                          drand_schedule,
                          o.pubsub_gate,
                          key_store);

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
