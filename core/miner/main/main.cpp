/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <libp2p/injector/host_injector.hpp>

#include "api/full_node/node_api.hpp"
#include "api/rpc/client_setup.hpp"
#include "api/rpc/info.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "api/storage_miner/storage_api.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/io_thread.hpp"
#include "common/libp2p/soralog.hpp"
#include "common/outcome.hpp"
#include "common/peer_key.hpp"
#include "config/profile_config.hpp"
#include "data_transfer/dt.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/retrieval/provider/impl/retrieval_provider_impl.hpp"
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "markets/storage/provider/impl/provider_impl.hpp"
#include "miner/impl/miner_impl.hpp"
#include "miner/mining.hpp"
#include "miner/windowpost.hpp"
#include "primitives/address/config.hpp"
#include "proofs/proof_param_provider.hpp"
#include "sector_storage/fetch_handler.hpp"
#include "sector_storage/impl/manager_impl.hpp"
#include "sector_storage/impl/scheduler_impl.hpp"
#include "sector_storage/stores/impl/index_impl.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/impl/remote_store.hpp"
#include "sector_storage/stores/impl/storage_impl.hpp"
#include "sectorblocks/impl/blocks_impl.hpp"
#include "storage/filestore/impl/filesystem/filesystem_filestore.hpp"
#include "storage/ipfs/graphsync/impl/graphsync_impl.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "storage/piece/impl/piece_storage_impl.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor.hpp"

namespace fc {
  using boost::asio::io_context;
  using common::span::cbytes;
  using config::configProfile;
  using libp2p::basic::Scheduler;
  using libp2p::multi::Multiaddress;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;
  using storage::BufferMap;
  namespace uuids = boost::uuids;

  static const Buffer kActor{cbytes("actor")};

  struct Config {
    boost::filesystem::path repo_path;
    std::pair<Multiaddress, std::string> node_api{
        codec::cbor::kDefaultT<Multiaddress>(), {}};
    boost::optional<Address> actor, owner, worker;
    boost::optional<RegisteredSealProof> seal_type;
    std::vector<Address> precommit_control;
    int api_port;

    /** Path to presealed sectors */
    boost::optional<boost::filesystem::path> preseal_path;

    auto join(const std::string &path) const {
      return (repo_path / path).string();
    }
  };

  outcome::result<Config> readConfig(int argc, char **argv) {
    namespace po = boost::program_options;
    Config config;
    struct {
      boost::filesystem::path node_repo;
      std::string sector_size;
    } raw;

    po::options_description desc("Fuhon miner options");
    auto option{desc.add_options()};
    option("miner-repo", po::value(&config.repo_path)->required());
    option("repo", po::value(&raw.node_repo));
    option("miner-api", po::value(&config.api_port)->default_value(2345));
    option("actor", po::value(&config.actor));
    option("owner", po::value(&config.owner));
    option("worker", po::value(&config.worker));
    option("sector-size", po::value(&raw.sector_size));
    option("precommit-control", po::value(&config.precommit_control));
    option("pre-sealed-sectors",
           po::value(&config.preseal_path),
           "Path to presealed sectors");
    desc.add(configProfile());
    primitives::address::configCurrentNetwork(option);

    po::variables_map vm;
    po::store(parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    boost::filesystem::create_directories(config.repo_path);
    std::ifstream config_file{config.join("config.cfg")};
    if (config_file.good()) {
      po::store(po::parse_config_file(config_file, desc), vm);
      po::notify(vm);
    }

    OUTCOME_TRYA(config.node_api,
                 api::rpc::loadInfo(raw.node_repo, "FULLNODE_API_INFO"));
    if (!raw.sector_size.empty()) {
      boost::algorithm::to_lower(raw.sector_size);
      if (raw.sector_size == "2kib") {
        config.seal_type = RegisteredSealProof::kStackedDrg2KiBV1;
      } else if (raw.sector_size == "8mib") {
        config.seal_type = RegisteredSealProof::kStackedDrg8MiBV1;
      } else if (raw.sector_size == "512mib") {
        config.seal_type = RegisteredSealProof::kStackedDrg512MiBV1;
      } else if (raw.sector_size == "32gib") {
        config.seal_type = RegisteredSealProof::kStackedDrg32GiBV1;
      } else if (raw.sector_size == "64gib") {
        config.seal_type = RegisteredSealProof::kStackedDrg64GiBV1;
      } else {
        spdlog::error("invalid --sector-size value");
        exit(EXIT_FAILURE);
      }
    }

    return config;
  }

  outcome::result<void> setupMiner(Config &config,
                                   BufferMap &kv,
                                   const PeerId &peer_id) {
    IoThread io_thread;
    io_context io;
    api::FullNodeApi api;
    api::rpc::Client wsc{*io_thread.io};
    wsc.setup(api);
    OUTCOME_TRY(
        wsc.connect(config.node_api.first, "/rpc/v0", config.node_api.second));

    Buffer _peer_id{peer_id.toVector()};
    if (!kv.contains(kActor)) {
      if (!config.actor) {
        spdlog::info("creating miner actor");
        OUTCOME_TRY(version, api.StateNetworkVersion({}));
        assert(config.seal_type);
        if (version >= api::NetworkVersion::kVersion7) {
          switch (*config.seal_type) {
            case RegisteredSealProof::kStackedDrg2KiBV1:
              config.seal_type = RegisteredSealProof::kStackedDrg2KiBV1_1;
              break;
            case RegisteredSealProof::kStackedDrg8MiBV1:
              config.seal_type = RegisteredSealProof::kStackedDrg8MiBV1_1;
              break;
            case RegisteredSealProof::kStackedDrg512MiBV1:
              config.seal_type = RegisteredSealProof::kStackedDrg512MiBV1_1;
              break;
            case RegisteredSealProof::kStackedDrg32GiBV1:
              config.seal_type = RegisteredSealProof::kStackedDrg32GiBV1_1;
              break;
            case RegisteredSealProof::kStackedDrg64GiBV1:
              config.seal_type = RegisteredSealProof::kStackedDrg64GiBV1_1;
              break;
            default:
              break;
          }
        }
        assert(config.owner);
        if (!config.worker) {
          config.worker = config.owner;
        }
        using vm::actor::builtin::v0::storage_power::CreateMiner;
        OUTCOME_TRY(params,
                    codec::cbor::encode(CreateMiner::Params{
                        *config.owner,
                        *config.worker,
                        *config.seal_type,
                        _peer_id,
                        {},
                    }));
        OUTCOME_TRY(smsg,
                    api.MpoolPushMessage({vm::actor::kStoragePowerAddress,
                                          *config.owner,
                                          {},
                                          {},
                                          {},
                                          {},
                                          CreateMiner::Number,
                                          params},
                                         api::kPushNoSpec));
        spdlog::info(
            "msg {}: CreateMiner owner={}", smsg.getCid(), *config.owner);
        // TODO: maybe async call, it's long
        OUTCOME_TRY(result,
                    api.StateWaitMsg(smsg.getCid(),
                                     kMessageConfidence,
                                     api::kLookbackNoLimit,
                                     true));
        if (result.receipt.exit_code != vm::VMExitCode::kOk) {
          spdlog::error("failed to create miner actor: {}",
                        result.receipt.exit_code);
          exit(EXIT_FAILURE);
        }
        OUTCOME_TRY(created,
                    codec::cbor::decode<CreateMiner::Result>(
                        result.receipt.return_value));
        config.actor = created.id_address;
        spdlog::info("created miner actor {}", *config.actor);
      }
      OUTCOME_TRY(kv.put(kActor, primitives::address::encode(*config.actor)));
    } else {
      OUTCOME_TRY(_actor, kv.get(kActor));
      OUTCOME_TRYA(config.actor, primitives::address::decode(_actor));
    }
    OUTCOME_TRY(minfo, api.StateMinerInfo(*config.actor, {}));
    config.owner = minfo.owner;
    config.worker = minfo.worker;
    if (minfo.peer_id.empty() || minfo.peer_id != _peer_id) {
      using vm::actor::builtin::v0::miner::ChangePeerId;
      OUTCOME_TRY(params, codec::cbor::encode(ChangePeerId::Params{_peer_id}));
      OUTCOME_TRY(smsg,
                  api.MpoolPushMessage({*config.actor,
                                        minfo.worker,
                                        {},
                                        {},
                                        {},
                                        {},
                                        ChangePeerId::Number,
                                        params},
                                       api::kPushNoSpec));
      spdlog::info(
          "msg {}: ChangePeerId peer={}", smsg.getCid(), peer_id.toBase58());

      api.StateWaitMsg(
          [](auto _res) {
            if (_res) {
              auto &receipt{_res.value().receipt};
              if (receipt.exit_code == vm::VMExitCode::kOk) {
                spdlog::info("ChangePeerId ok");
              } else {
                spdlog::info("ChangePeerId {}", receipt.exit_code);
              }
            } else {
              spdlog::warn("ChangePeerId error {}", _res.error());
            }
          },
          smsg.getCid(),
          kMessageConfidence,
          api::kLookbackNoLimit,
          true);
    }

    OUTCOME_TRY(
        params,
        proofs::ProofParamProvider::readJson(config.join("proof-params.json")));
    OUTCOME_TRY(
        proofs::ProofParamProvider::getParams(params, minfo.sector_size));

    return outcome::success();
  }

  outcome::result<void> main(Config &config) {
    auto clock{std::make_shared<clock::UTCClockImpl>()};

    OUTCOME_TRY(leveldb, storage::LevelDB::create(config.join("leveldb")));
    auto prefixed{[&](auto s) {
      return std::make_shared<storage::MapPrefix>(s, leveldb);
    }};
    auto one_key{[&](auto s, auto m) {
      return std::make_shared<storage::OneKey>(s, m);
    }};

    OUTCOME_TRY(peer_key, loadPeerKey(config.join("peer_ed25519.key")));

    auto injector{libp2p::injector::makeHostInjector(
        libp2p::injector::useKeyPair(peer_key))};
    auto io{injector.create<std::shared_ptr<io_context>>()};
    auto host{injector.create<std::shared_ptr<libp2p::Host>>()};
    auto scheduler{injector.create<std::shared_ptr<Scheduler>>()};

    IoThread sealing_thread;

    OUTCOME_TRY(setupMiner(config, *leveldb, host->getId()));

    auto napi{std::make_shared<api::FullNodeApi>()};
    api::rpc::Client wsc{*io};
    wsc.setup(*napi);
    OUTCOME_TRY(
        wsc.connect(config.node_api.first, "/rpc/v0", config.node_api.second));

    host->start();
    OUTCOME_TRY(node_peer, napi->NetAddrsListen());
    host->connect(node_peer);

    auto storage{std::make_shared<sector_storage::stores::LocalStorageImpl>(
        config.repo_path.string())};
    OUTCOME_TRY(storage->setStorage([&](auto &storage_config) {
      if (storage_config.storage_paths.empty()) {
        boost::filesystem::path path{config.join("sectors")};
        OUTCOME_EXCEPT(common::writeFile(
            path / sector_storage::stores::kMetaFileName,
            *codec::json::format(api::encode(primitives::LocalStorageMeta{
                uuids::to_string(uuids::random_generator()()),
                kDefaultStorageWeight,
                true,
                true,
            }))));
        storage_config.storage_paths.push_back({path.string()});
      }
      if (config.preseal_path
          && !storage_config.has(config.preseal_path->string())) {
        storage_config.storage_paths.push_back({config.preseal_path->string()});
      }
    }));

    auto sector_index{
        std::make_shared<sector_storage::stores::SectorIndexImpl>()};

    OUTCOME_TRY(local_store,
                sector_storage::stores::LocalStoreImpl::newLocalStore(
                    storage,
                    sector_index,
                    std::vector<std::string>{"http://127.0.0.1"},
                    scheduler));

    // TODO: auth headers should be here
    auto remote_store{std::make_shared<sector_storage::stores::RemoteStoreImpl>(
        local_store, std::unordered_map<std::string, std::string>{})};

    OUTCOME_TRY(
        wscheduler,
        sector_storage::SchedulerImpl::newScheduler(
            io, prefixed("scheduler_works/")));  // maybe use another io_context
    OUTCOME_TRY(manager,
                sector_storage::ManagerImpl::newManager(
                    io, remote_store, wscheduler, {true, true, true, true}));

    // TODO(ortyomka): make param
    mining::Config default_config{.max_wait_deals_sectors = 2,
                                  .max_sealing_sectors = 0,
                                  .max_sealing_sectors_for_deals = 0,
                                  .wait_deals_delay = std::chrono::hours(6)};
    OUTCOME_TRY(
        miner,
        miner::MinerImpl::newMiner(napi,
                                   *config.actor,
                                   *config.worker,
                                   std::make_shared<primitives::StoredCounter>(
                                       leveldb, "sector_counter"),
                                   prefixed("sealing_fsm/"),
                                   manager,
                                   scheduler,
                                   sealing_thread.io,
                                   default_config,
                                   config.precommit_control));
    auto sealing{miner->getSealing()};

    OUTCOME_TRY(
        mining,
        mining::Mining::create(scheduler, clock, napi, manager, *config.actor));
    mining->start();

    auto window{mining::WindowPoStScheduler::create(
        napi, manager, manager, *config.actor)};

    auto graphsync{std::make_shared<storage::ipfs::graphsync::GraphsyncImpl>(
        host, scheduler)};
    graphsync->start();
    auto datatransfer{data_transfer::DataTransfer::make(host, graphsync)};

    auto markets_ipld{std::make_shared<storage::ipfs::LeveldbDatastore>(
        prefixed("markets_ipld/"))};
    auto gs_sub{graphsync->subscribe([&](auto &, auto &data) {
      OUTCOME_EXCEPT(markets_ipld->set(data.cid, data.content));
    })};
    auto stored_ask{std::make_shared<markets::storage::provider::StoredAsk>(
        prefixed("stored_ask/"), napi, *config.actor)};
    auto piece_storage{std::make_shared<storage::piece::PieceStorageImpl>(
        prefixed("storage_provider/"))};
    auto sector_blocks{std::make_shared<sectorblocks::SectorBlocksImpl>(miner)};
    auto chain_events{
        std::make_shared<markets::storage::chain_events::ChainEventsImpl>(
            napi)};
    OUTCOME_TRY(chain_events->init());
    auto piece_io{std::make_shared<markets::pieceio::PieceIOImpl>(
        config.join("piece_io"))};
    auto filestore{std::make_shared<storage::filestore::FileSystemFileStore>()};
    auto storage_provider{
        std::make_shared<markets::storage::provider::StorageProviderImpl>(
            host,
            markets_ipld,
            datatransfer,
            stored_ask,
            io,
            piece_storage,
            napi,
            sector_blocks,
            chain_events,
            *config.actor,
            piece_io,
            filestore)};
    OUTCOME_TRY(storage_provider->init());
    auto retrieval_provider{
        std::make_shared<markets::retrieval::provider::RetrievalProviderImpl>(
            host,
            datatransfer,
            napi,
            piece_storage,
            one_key("retrieval_provider_ask", leveldb),
            manager,
            miner)};
    retrieval_provider->start();

    auto mapi = api::makeStorageApi(io,
                                    napi,
                                    *config.actor,
                                    miner,
                                    sector_index,
                                    manager,
                                    wscheduler,
                                    stored_ask,
                                    storage_provider,
                                    retrieval_provider);

    std::map<std::string, std::shared_ptr<api::Rpc>> mrpc;
    mrpc.emplace("/rpc/v0", api::makeRpc(*mapi));
    auto mroutes{std::make_shared<api::Routes>()};

    mroutes->insert({"/remote", sector_storage::serveHttp(local_store)});

    api::serve(mrpc, mroutes, *io, "127.0.0.1", config.api_port);
    api::rpc::saveInfo(config.repo_path, config.api_port, "stub");

    spdlog::info("fuhon miner started");
    spdlog::info("peer id {}", host->getId().toBase58());

    io->run();
    return outcome::success();
  }
}  // namespace fc

int main(int argc, char **argv) {
  fc::libp2pSoralog();

  OUTCOME_EXCEPT(config, fc::readConfig(argc, argv));
  if (const auto res{fc::main(config)}; !res) {
    spdlog::error("main: {:#}", res.error());
  }
}
