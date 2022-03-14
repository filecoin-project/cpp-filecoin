/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/io_context.hpp>
#include <boost/di/extension/scopes/shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <set>

#include "api/rpc/client_setup.hpp"
#include "api/rpc/info.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "api/storage_miner/storage_api.hpp"
#include "api/worker_api.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/io_thread.hpp"
#include "common/outcome.hpp"
#include "config/profile_config.hpp"
#include "primitives/address/config.hpp"
#include "proofs/proof_param_provider.hpp"
#include "remote_worker/remote_worker_api.hpp"
#include "sector_storage/fetch_handler.hpp"
#include "sector_storage/impl/local_worker.hpp"
#include "sector_storage/stores/impl/remote_index_impl.hpp"
#include "sector_storage/stores/impl/remote_store.hpp"
#include "sector_storage/stores/impl/storage_impl.hpp"

namespace fc::remote_worker {
  using api::kMinerApiVersion;
  using boost::asio::io_context;
  using config::configProfile;
  using libp2p::multi::Multiaddress;
  using sector_storage::LocalWorker;
  using sector_storage::stores::LocalStore;
  namespace uuids = boost::uuids;

  /** Required Miner API version */
  const auto kExpectedMinerApiVersion = kMinerApiVersion;

  struct Config {
    boost::filesystem::path repo_path;
    std::pair<Multiaddress, std::string> miner_api{
        codec::cbor::kDefaultT<Multiaddress>(), {}};
    int api_port{};

    std::set<primitives::TaskType> tasks;
    bool need_download = false;

    auto join(const std::string &path) const {
      return (repo_path / path).string();
    }
  };

  outcome::result<Config> readConfig(int argc, char **argv) {
    namespace po = boost::program_options;
    Config config;
    struct {
      boost::filesystem::path miner_repo;
      bool can_add_piece = false;
      bool can_precommit1 = false;
      bool can_precommit2 = false;
      bool can_commit = false;
      bool can_unseal = false;
      bool can_replica_update = false;
      bool can_prove_replica_update2 = false;
    } raw;

    po::options_description desc("Fuhon worker options");
    auto option{desc.add_options()};
    option("worker-repo", po::value(&config.repo_path)->required());
    option("miner-repo", po::value(&raw.miner_repo));
    option("worker-api", po::value(&config.api_port)->default_value(3456));
    option("addpiece", po::value(&raw.can_add_piece)->default_value(true));
    option("precommit1", po::value(&raw.can_precommit1)->default_value(true));
    option("precommit2", po::value(&raw.can_precommit2)->default_value(true));
    option("commit", po::value(&raw.can_commit)->default_value(true));
    option("unseal", po::value(&raw.can_unseal)->default_value(true));
    option("replica-update", po::value(&raw.can_replica_update));
    option("prove-replica-update2", po::value(&raw.can_prove_replica_update2));
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

    OUTCOME_TRYA(config.miner_api,
                 api::rpc::loadInfo(raw.miner_repo, "MINER_API_INFO"));
    config.tasks = {
        primitives::kTTFetch, primitives::kTTCommit1, primitives::kTTFinalize};

    if (raw.can_add_piece) {
      // TODO(ortyomka): [FIL-344] add kTTAddPiece to tasks
      spdlog::warn("AddPiece function is not available");
    }
    if (raw.can_precommit1) {
      config.tasks.insert(primitives::kTTPreCommit1);
    }
    if (raw.can_precommit2) {
      config.tasks.insert(primitives::kTTPreCommit2);
    }
    if (raw.can_commit) {
      config.need_download = true;
      config.tasks.insert(primitives::kTTCommit2);
    }
    if (raw.can_unseal) {
      config.tasks.insert(primitives::kTTUnseal);
    }
    if (raw.can_replica_update) {
      config.tasks.insert(primitives::kTTReplicaUpdate);
    }
    if (raw.can_prove_replica_update2) {
      config.tasks.insert(primitives::kTTProveReplicaUpdate2);
    }

    return config;
  }

  outcome::result<void> main(Config &config) {
    auto io{std::make_shared<io_context>()};
    auto injector =
        libp2p::injector::makeHostInjector<boost::di::extension::shared_config>(
            boost::di::bind<boost::asio::io_context>.template to(
                io)[boost::di::override]);
    auto scheduler{
        injector.create<std::shared_ptr<libp2p::basic::Scheduler>>()};

    auto mapi{std::make_shared<api::StorageMinerApi>()};
    api::rpc::Client wsc{*io};
    wsc.setup(*mapi);
    OUTCOME_TRY(wsc.connect(
        config.miner_api.first, "/rpc/v0", config.miner_api.second));

    OUTCOME_TRY(version, mapi->Version());

    if (version.api_version != kExpectedMinerApiVersion) {
      spdlog::error("lotus-miner API version doesn't match: expected: {}",
                    kExpectedMinerApiVersion);
      exit(EXIT_FAILURE);
    }

    if (config.need_download) {
      OUTCOME_TRY(address, mapi->ActorAddress());
      OUTCOME_TRY(sector_size, mapi->ActorSectorSize(address));

      OUTCOME_TRY(
          proofs::getParams(config.join("proof-params.json"), sector_size));
    }

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
                false,
            }))));
        storage_config.storage_paths.push_back({path.string()});
      }
    }));

    auto index_adapter =
        std::make_shared<sector_storage::stores::RemoteSectorIndexImpl>(mapi);

    OUTCOME_TRY(
        local_store,
        sector_storage::stores::LocalStoreImpl::newLocalStore(
            storage,
            index_adapter,
            std::vector<std::string>{"http://127.0.0.1/remote"},  // TODO: API
            scheduler));

    OUTCOME_TRY(admin_token, mapi->AuthNew({api::kAdminPermission}));

    std::unordered_map<std::string, std::string> auth_headers;
    auth_headers["Authorization"] =
        "Bearer " + std::string(admin_token.begin(), admin_token.end());
    auto remote_store{std::make_shared<sector_storage::stores::RemoteStoreImpl>(
        local_store, std::move(auth_headers))};

    sector_storage::WorkerConfig wconfig{
        .custom_hostname = boost::none,  // TODO: add flag for change it
        .task_types = config.tasks,
        .is_no_swap = false,  // TODO: add flag for change it
    };

    auto worker{std::make_shared<LocalWorker>(io, wconfig, mapi, remote_store)};

    auto worker_api = makeWorkerApi(local_store, worker);

    std::map<std::string, std::shared_ptr<api::Rpc>> wrpc;
    wrpc.emplace("/rpc/v0", api::makeRpc(*worker_api));
    auto wroutes{std::make_shared<api::Routes>()};

    // TODO[@Elestrias] - Fuhon remote worker AddPiece

    wroutes->insert({"/remote",
                     api::makeAuthRoute(
                         sector_storage::serveHttp(local_store),
                         std::bind(mapi->AuthVerify, std::placeholders::_1))});

    api::serve(wrpc, wroutes, *io, "127.0.0.1", config.api_port);
    OUTCOME_TRY(token, mapi->AuthNew(primitives::jwt::kAllPermission));
    api::rpc::saveInfo(config.repo_path,
                       config.api_port,
                       std::string(token.begin(), token.end()));

    IoThread thread;
    boost::asio::post(*(thread.io), [&] {
      spdlog::info("fuhon worker are registering");
      auto address{fmt::format("/ip4/127.0.0.1/tcp/{}/http", config.api_port)};
      mapi->WorkerConnect(
          [](const outcome::result<void> &maybe_error) {
            if (maybe_error.has_error()) {
              spdlog::error("worker register error: {}",
                            maybe_error.error().message());
              return;
            }
            spdlog::info("fuhon worker registered");
          },
          address);  // TODO: make reconnect if failed
    });

    spdlog::info("fuhon worker started");
    io->run();
    return outcome::success();
  }
}  // namespace fc::remote_worker

int main(int argc, char **argv) {
  OUTCOME_EXCEPT(config, fc::remote_worker::readConfig(argc, argv));
  OUTCOME_EXCEPT(fc::remote_worker::main(config));
}
