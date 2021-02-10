/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "builder.hpp"

#include <boost/di/extension/scopes/shared.hpp>
#include <libp2p/injector/host_injector.hpp>

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
#include "node/blocksync_server.hpp"
#include "node/chain_store_impl.hpp"
#include "node/graphsync_server.hpp"
#include "node/identify.hpp"
#include "node/interpret_job.hpp"
#include "node/peer_discovery.hpp"
#include "node/pubsub_gate.hpp"
#include "node/receive_hello.hpp"
#include "node/say_hello.hpp"
#include "node/sync_job.hpp"
#include "power/impl/power_table_impl.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/car/car.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::node {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger.get();
    }

    outcome::result<void> loadCar(storage::ipfs::IpfsDatastore &storage,
                                  Config &config) {
      std::ifstream file{config.car_file_name,
                         std::ios::binary | std::ios::ate};
      if (!file.good()) {
        log()->error("cannot open file {}", config.car_file_name);
        return Error::CAR_FILE_OPEN_ERROR;
      }

      static const size_t kMaxSize = 64 * 1024 * 1024;
      auto size = static_cast<size_t>(file.tellg());
      if (size > kMaxSize) {
        log()->error("car file size above expected, file:{}, size:{}, limit:{}",
                     config.car_file_name,
                     size,
                     kMaxSize);
        return Error::CAR_FILE_SIZE_ABOVE_LIMIT;
      }

      std::string buffer;
      buffer.resize(size);
      file.seekg(0, std::ios::beg);
      file.read(buffer.data(), buffer.size());

      auto result =
          fc::storage::car::loadCar(storage, common::span::cbytes(buffer));
      if (!result) {
        log()->error("cannot load car file {}: {}",
                     config.car_file_name,
                     result.error().message());
        return result.error();
      }

      const auto &roots = result.value();
      if (roots.empty()) {
        return Error::NO_GENESIS_BLOCK;
      }

      if (config.genesis_cid) {
        if (config.genesis_cid.value() != roots[0]) {
          log()->error("Genesis mismatch: got cids:{}, expected:{}",
                       fmt::join(roots, " "),
                       config.genesis_cid.value().toString().value());
          return Error::GENESIS_MISMATCH;
        }
      } else {
        config.genesis_cid = roots[0];
        log()->debug("Genesis found in {}: {}",
                     config.car_file_name,
                     config.genesis_cid.value().toString().value());
      }

      return fc::outcome::success();
    }

    outcome::result<void> initNetworkName(
        const primitives::tipset::Tipset &genesis_tipset,
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        Config &config) {
      OUTCOME_TRY(init_state,
                  vm::state::StateTreeImpl(
                      ipld, genesis_tipset.blks[0].parent_state_root)
                      .state<vm::actor::builtin::v0::init::InitActorState>(
                          vm::actor::kInitAddress));
      config.network_name = init_state.network_name;
      return outcome::success();
    }

    std::shared_ptr<libp2p::protocol::kademlia::KademliaImpl> createKademlia(
        Config &config,
        const NodeObjects &o,
        std::shared_ptr<libp2p::peer::IdentityManager> id_manager,
        std::shared_ptr<libp2p::event::Bus> bus) {
      config.kademlia_config.protocolId =
          std::string("/fil/kad/") + config.network_name + "/kad/1.0.0";

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

  outcome::result<NodeObjects> createNodeObjects(Config &config) {
    NodeObjects o;

    log()->debug("Creating storage...");

    bool creating_new_db = false;

    if (config.repo_path == "memory") {
      o.ipld = std::make_shared<storage::ipfs::InMemoryDatastore>();
      o.kv_store = std::make_shared<storage::InMemoryStorage>();
      creating_new_db = true;
    } else {
      leveldb::Options options;
      if (!config.car_file_name.empty()) {
        options.create_if_missing = true;
        options.error_if_exists = true;
        creating_new_db = true;
      }
      auto leveldb_res = storage::LevelDB::create(config.join(kLeveldbPath),
                                                  std::move(options));
      if (!leveldb_res) {
        return Error::STORAGE_INIT_ERROR;
      }
      o.ipld = makeIpld(leveldb_res.value());
      o.kv_store = std::move(leveldb_res.value());
    }
    o.ts_load = std::make_shared<primitives::tipset::TsLoadCache>(
        std::make_shared<primitives::tipset::TsLoadIpld>(o.ipld), 8 << 10);

    if (creating_new_db) {
      log()->debug("Loading initial car file...");
      OUTCOME_TRY(loadCar(*o.ipld, config));
    }

    log()->debug("Creating chain DB...");

    OUTCOME_EXCEPT(genesis, o.ts_load->load(TipsetKey{{*config.genesis_cid}}));
    OUTCOME_TRY(initNetworkName(*genesis, o.ipld, config));
    log()->info("Network name: {}", config.network_name);

    auto genesis_timestamp = clock::UnixTime(genesis->blks[0].timestamp);

    log()->info("Genesis: {}, timestamp {}",
                config.genesis_cid.value().toString().value(),
                clock::unixTimeToString(genesis_timestamp));

    o.utc_clock = std::make_shared<clock::UTCClockImpl>();

    o.chain_epoch_clock =
        std::make_shared<clock::ChainEpochClockImpl>(genesis_timestamp);

    log()->debug("Creating host...");

    OUTCOME_TRY(keypair, loadPeerKey(config.join(kPeerKeyPath)));

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
        o.host, o.ts_load, o.ipld);

    OUTCOME_TRY(circulating,
                vm::Circulating::make(o.ipld, config.genesis_cid.value()));

    o.vm_interpreter = std::make_shared<vm::interpreter::CachedInterpreter>(
        std::make_shared<vm::interpreter::InterpreterImpl>(
            std::make_shared<vm::actor::InvokerImpl>(),
            o.ts_load,
            std::make_shared<blockchain::weight::WeightCalculatorImpl>(o.ipld),
            std::make_shared<vm::runtime::TipsetRandomness>(o.ts_load),
            std::move(circulating)),
        std::make_shared<storage::MapPrefix>(kCachedInterpreterPrefix,
                                             o.kv_store));

    auto weight_calculator =
        std::make_shared<blockchain::weight::WeightCalculatorImpl>(o.ipld);

    o.interpret_job = sync::InterpretJob::create(
        o.vm_interpreter, o.ts_branches, o.ipld, o.events);

    o.sync_job = std::make_shared<sync::SyncJob>(o.host,
                                                 o.scheduler,
                                                 o.interpret_job,
                                                 o.ts_branches,
                                                 o.ts_main,
                                                 o.ts_load,
                                                 o.ipld);

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
            o.vm_interpreter);

    o.chain_store =
        std::make_shared<sync::ChainStoreImpl>(o.ipld,
                                               o.vm_interpreter,
                                               weight_calculator,
                                               std::move(block_validator));

    log()->debug("Creating API...");

    auto mpool = storage::mpool::Mpool::create(
        o.ts_load, o.ts_main, o.ipld, o.vm_interpreter, o.chain_store);

    auto msg_waiter = storage::blockchain::MsgWaiter::create(
        o.ts_load, o.ipld, o.chain_store);

    auto key_store = std::make_shared<storage::keystore::InMemoryKeyStore>(
        bls_provider, secp_provider);

    drand::ChainInfo drand_chain_info{
        .key = config.drand_bls_pubkey,
        .genesis = std::chrono::seconds(config.drand_genesis),
        .period = std::chrono::seconds(config.drand_period),
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

    o.api = std::make_shared<api::Api>(api::makeImpl(o.chain_store,
                                                     config.network_name,
                                                     weight_calculator,
                                                     o.ts_load,
                                                     o.ts_main,
                                                     o.ipld,
                                                     mpool,
                                                     o.vm_interpreter,
                                                     msg_waiter,
                                                     beaconizer,
                                                     drand_schedule,
                                                     o.pubsub_gate,
                                                     key_store));

    return o;
  }

  IpldPtr makeIpld(std::shared_ptr<storage::PersistentBufferMap> map) {
    return std::make_shared<storage::ipfs::LeveldbDatastore>(
        std::make_shared<storage::MapPrefix>("ipld", map));
  }
}  // namespace fc::node

OUTCOME_CPP_DEFINE_CATEGORY(fc::node, Error, e) {
  using E = fc::node::Error;

  switch (e) {
    case E::STORAGE_INIT_ERROR:
      return "cannot initialize storage";
    case E::CAR_FILE_OPEN_ERROR:
      return "cannot open initial car file";
    case E::CAR_FILE_SIZE_ABOVE_LIMIT:
      return "car file size above limit";
    case E::NO_GENESIS_BLOCK:
      return "no genesis block";
    case E::GENESIS_MISMATCH:
      return "genesis mismatch";
    default:
      break;
  }
  return "node::Error: unknown error";
}
