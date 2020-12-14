/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <libp2p/injector/host_injector.hpp>

#include "api/rpc/info.hpp"
#include "api/rpc/json.hpp"
#include "api/rpc/ws.hpp"
#include "api/rpc/wsc.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/peer_key.hpp"
#include "miner/impl/miner_impl.hpp"
#include "miner/mining.hpp"
#include "miner/windowpost.hpp"
#include "proofs/proof_param_provider.hpp"
#include "sector_storage/impl/manager_impl.hpp"
#include "sector_storage/impl/scheduler_impl.hpp"
#include "sector_storage/stores/impl/index_impl.hpp"
#include "sector_storage/stores/impl/local_store.hpp"
#include "sector_storage/stores/impl/storage_impl.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_export.hpp"

namespace fc {
  using api::Address;
  using api::RegisteredProof;
  using boost::asio::io_context;
  using common::span::cbytes;
  using libp2p::multi::Multiaddress;
  using storage::BufferMap;
  using storage::MapPrefix;

  static const Buffer kActor{cbytes("actor")};

  struct Config {
    boost::filesystem::path repo_path;
    std::pair<Multiaddress, std::string> node_api{
        codec::cbor::kDefaultT<Multiaddress>(), {}};
    boost::optional<Address> actor, owner, worker;
    RegisteredProof seal_type{RegisteredProof::StackedDRG2KiBSeal};
    int api_port;

    auto join(const std::string &path) const {
      return (repo_path / path).string();
    }
  };

  outcome::result<Config> readConfig(int argc, char **argv) {
    namespace po = boost::program_options;
    Config config;
    struct {
      std::string repo;
      std::string node_repo;
      std::string actor, owner, worker;
    } raw;

    po::options_description desc("Fuhon miner options");
    auto option{desc.add_options()};
    option("miner-repo", po::value(&raw.repo)->required());
    option("repo", po::value(&raw.node_repo));
    option("miner-api", po::value(&config.api_port)->default_value(2345));
    option("actor", po::value(&raw.actor));
    option("owner", po::value(&raw.owner));
    option("worker", po::value(&raw.worker));

    po::variables_map vm;
    po::store(parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    config.repo_path = raw.repo;
    if (!boost::filesystem::exists(config.repo_path)) {
      boost::filesystem::create_directories(config.repo_path);
    }
    std::ifstream config_file{config.join("config.cfg")};
    if (config_file.good()) {
      po::store(po::parse_config_file(config_file, desc), vm);
      po::notify(vm);
    }

    auto address{[&](auto &out, auto &str) {
      if (str.empty()) {
        out.reset();
      } else {
        out = primitives::address::decodeFromString(str).value();
      }
    }};
    OUTCOME_TRYA(config.node_api,
                 api::rpc::loadInfo(raw.node_repo, "FULLNODE_API_INFO"));
    address(config.actor, raw.actor);
    address(config.owner, raw.owner);
    address(config.worker, raw.worker);

    return config;
  }

  outcome::result<void> setupMiner(Config &config,
                                   BufferMap &kv,
                                   const PeerId &peer_id) {
    OUTCOME_TRY(
        params,
        proofs::ProofParamProvider::readJson(config.join("proof-params.json")));
    OUTCOME_TRY(sector_size,
                primitives::sector::getSectorSize(config.seal_type));
    OUTCOME_TRY(proofs::ProofParamProvider::getParams(params, sector_size));

    io_context io;
    boost::asio::executor_work_guard<io_context::executor_type> work_guard{
        io.get_executor()};
    std::thread thread{[&] { io.run(); }};
    auto BOOST_OUTCOME_TRY_UNIQUE_NAME{gsl::finally([&] {
      io.stop();
      thread.join();
    })};
    api::Api api;
    api::rpc::Client wsc{io};
    wsc.setup(api);
    OUTCOME_TRY(wsc.connect(config.node_api.first, config.node_api.second));

    Buffer _peer_id{peer_id.toVector()};
    if (!kv.contains(kActor)) {
      if (!config.actor) {
        spdlog::info("creating miner actor");
        OUTCOME_TRY(version, api.StateNetworkVersion({}));
        if (version >= api::NetworkVersion{7}) {
          spdlog::warn("TODO: (network.v7) check new seal-type");
          exit(-1);
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
                        config.seal_type,
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
        OUTCOME_TRY(wait, api.StateWaitMsg(smsg.getCid(), kMessageConfidence));
        OUTCOME_TRY(result, wait.waitSync());
        assert(result.receipt.exit_code == vm::VMExitCode::kOk);
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
      OUTCOME_TRY(wait, api.StateWaitMsg(smsg.getCid(), kMessageConfidence));
      wait.waitOwn([](auto _res) {
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
      });
    }
    return outcome::success();
  }

  outcome::result<void> main(Config &config) {
    auto clock{std::make_shared<clock::UTCClockImpl>()};

    OUTCOME_TRY(leveldb, storage::LevelDB::create(config.join("leveldb")));

    OUTCOME_TRY(peer_key, loadPeerKey(config.join("peer_ed25519.key")));

    auto injector{libp2p::injector::makeHostInjector(
        libp2p::injector::useKeyPair(peer_key))};
    auto io{injector.create<std::shared_ptr<io_context>>()};
    auto host{injector.create<std::shared_ptr<libp2p::Host>>()};
    auto scheduler{
        injector.create<std::shared_ptr<libp2p::protocol::AsioScheduler>>()};

    OUTCOME_TRY(setupMiner(config, *leveldb, host->getId()));

    auto napi{std::make_shared<api::Api>()};
    api::rpc::Client wsc{*io};
    wsc.setup(*napi);
    OUTCOME_TRY(wsc.connect(config.node_api.first, config.node_api.second));

    host->start();
    OUTCOME_TRY(node_peer, napi->NetAddrsListen());
    host->connect(node_peer);

    auto storage{std::make_shared<sector_storage::stores::LocalStorageImpl>(
        config.join("storage.json"))};
    OUTCOME_TRY(storage->setStorage([&](auto &config2) {
      if (config2.storage_paths.empty()) {
        boost::filesystem::path path{config.join("sectors")};
        boost::filesystem::create_directories(path);
        // TODO(artyom-yurin): check meta
        OUTCOME_EXCEPT(common::writeFile(
            (path / sector_storage::stores::kMetaFileName).string(),
            *codec::json::format(api::encode(primitives::LocalStorageMeta{
                "default",
                1,
                true,
                true,
            }))));
        config2.storage_paths.push_back({path.string()});
      }
    }));

    OUTCOME_TRY(local_store,
                sector_storage::stores::LocalStoreImpl::newLocalStore(
                    storage,
                    std::make_shared<sector_storage::stores::SectorIndexImpl>(),
                    std::vector<std::string>{"http://127.0.0.1"},
                    scheduler));
    OUTCOME_TRY(
        manager,
        sector_storage::ManagerImpl::newManager(
            std::make_shared<sector_storage::stores::RemoteStoreImpl>(
                local_store, std::unordered_map<std::string, std::string>{}),
            std::make_shared<sector_storage::SchedulerImpl>(config.seal_type),
            {true, true, true, true}));
    auto miner{std::make_shared<miner::MinerImpl>(
        napi,
        *config.actor,
        *config.worker,
        std::make_shared<primitives::StoredCounter>(leveldb, "sector_counter"),
        manager,
        io)};
    OUTCOME_TRY(miner->run());
    auto sealing{miner->getSealing()};

    OUTCOME_TRY(
        mining,
        mining::Mining::create(scheduler, clock, napi, manager, *config.actor));
    mining->start();

    auto window{mining::WindowPoStScheduler::create(
        napi, manager, manager, *config.actor)};

    auto mapi{std::make_shared<api::Api>()};
    mapi->PledgeSector = [&]() -> outcome::result<void> {
      return sealing->pledgeSector();
    };
    api::serve(mapi, *io, "127.0.0.1", config.api_port);
    api::rpc::saveInfo(config.repo_path, config.api_port, "stub");

    spdlog::info("fuhon miner started");

    io->run();
    return outcome::success();
  }
}  // namespace fc

int main(int argc, char **argv) {
  OUTCOME_EXCEPT(config, fc::readConfig(argc, argv));
  OUTCOME_EXCEPT(fc::main(config));
}
