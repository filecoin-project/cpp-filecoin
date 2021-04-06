/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/io_context.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <set>

#include "api/rpc/client_setup.hpp"
#include "api/rpc/info.hpp"
#include "api/rpc/make.hpp"
#include "api/rpc/ws.hpp"
#include "api/storage_api.hpp"
#include "api/worker_api.hpp"
#include "codec/json/json.hpp"
#include "common/file.hpp"
#include "common/io_thread.hpp"
#include "common/outcome.hpp"
#include "config/profile_config.hpp"
#include "primitives/address/config.hpp"
#include "proofs/proof_param_provider.hpp"
#include "sector_storage/fetch_handler.hpp"
#include "sector_storage/impl/local_worker.hpp"
#include "sector_storage/stores/impl/remote_index_impl.hpp"
#include "sector_storage/stores/impl/remote_store.hpp"
#include "sector_storage/stores/impl/storage_impl.hpp"

namespace fc {
  using api::VersionResult;
  using boost::asio::io_context;
  using config::configProfile;
  using primitives::piece::PieceInfo;
  using primitives::piece::UnpaddedByteIndex;
  using primitives::piece::UnpaddedPieceSize;
  using primitives::sector::SealRandomness;
  using proofs::ProofParamProvider;
  using sector_storage::AcquireMode;
  using sector_storage::Commit1Output;
  using sector_storage::InteractiveRandomness;
  using sector_storage::LocalWorker;
  using sector_storage::PathType;
  using sector_storage::PreCommit1Output;
  using sector_storage::Range;
  using sector_storage::SectorCids;
  using sector_storage::SectorFileType;
  using sector_storage::stores::LocalStore;
  namespace uuids = boost::uuids;

  struct Config {
    boost::filesystem::path repo_path;
    std::pair<Multiaddress, std::string> miner_api{
        codec::cbor::kDefaultT<Multiaddress>(), {}};
    RegisteredSealProof seal_type;
    int api_port;

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

    return config;
  }

  outcome::result<void> main(Config &config) {
    auto io{std::make_shared<io_context>()};
    auto scheduler{std::make_shared<libp2p::protocol::AsioScheduler>(
        *io, libp2p::protocol::SchedulerConfig())};

    auto mapi{std::make_shared<api::StorageMinerApi>()};
    api::rpc::Client wsc{*io};
    wsc.setup(*mapi);
    OUTCOME_TRY(wsc.connect(config.miner_api.first, config.miner_api.second));

    OUTCOME_TRY(version, mapi->Version());

    if (version.api_version != kMinerApiVersion) {
      spdlog::error("lotus-miner API version doesn't match: expected: {}",
                    kMinerApiVersion);
      exit(EXIT_FAILURE);
    }

    // TODO(ortyomka): [FIL-347] remove it
    OUTCOME_TRYA(config.seal_type, mapi->SealProof());

    if (config.need_download) {
      OUTCOME_TRY(address, mapi->ActorAddress());
      OUTCOME_TRY(sector_size, mapi->ActorSectorSize(address));

      OUTCOME_TRY(
          params,
          ProofParamProvider::readJson(config.join("proof-params.json")));
      OUTCOME_TRY(ProofParamProvider::getParams(params, sector_size));
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

    OUTCOME_TRY(local_store,
                sector_storage::stores::LocalStoreImpl::newLocalStore(
                    storage,
                    index_adapter,
                    std::vector<std::string>{"http://127.0.0.1/remote"},
                    scheduler));

    // TODO: auth headers should be here
    auto remote_store{std::make_shared<sector_storage::stores::RemoteStoreImpl>(
        local_store, std::unordered_map<std::string, std::string>{})};

    sector_storage::WorkerConfig wconfig{
        .seal_proof_type = config.seal_type,
        .task_types = config.tasks,
    };

    auto worker{std::make_unique<LocalWorker>(wconfig, remote_store)};

    auto wapi{std::make_shared<api::WorkerApi>()};
    wapi->Version = []() { return VersionResult{"seal-worker", 0, 0}; };
    wapi->StorageAddLocal = [&](const std::string &path) {
      return local_store->openPath(path);
    };
    wapi->Fetch = [&](const SectorId &sector,
                      const SectorFileType &file_type,
                      PathType path_type,
                      AcquireMode mode) {
      return worker->fetch(sector, file_type, path_type, mode);
    };
    wapi->UnsealPiece = [&](const SectorId &sector,
                            UnpaddedByteIndex offset,
                            const UnpaddedPieceSize &size,
                            const SealRandomness &randomness,
                            const CID &unsealed_cid) {
      return worker->unsealPiece(
          sector, offset, size, randomness, unsealed_cid);
    };
    wapi->Remove = [&](const SectorId &sector) {
      return worker->remove(sector);
    };
    wapi->MoveStorage = [&](const SectorId &sector) {
      return worker->moveStorage(sector);
    };

    wapi->Info = [&]() { return worker->getInfo(); };
    wapi->Paths = [&]() { return worker->getAccessiblePaths(); };
    wapi->TaskTypes = [&]() -> outcome::result<std::set<primitives::TaskType>> {
      OUTCOME_TRY(tasks, worker->getSupportedTask());
      // TODO(ortyomka): [FIL-344] Remove its
      tasks.extract(primitives::kTTAddPiece);
      return std::move(tasks);
    };

    wapi->SealPreCommit1 = [&](const SectorId &sector,
                               const SealRandomness &ticket,
                               std::vector<PieceInfo> pieces) {
      return worker->sealPreCommit1(sector, ticket, pieces);
    };
    wapi->SealPreCommit2 = [&](const SectorId &sector,
                               const PreCommit1Output &pre_commit_1_output) {
      return worker->sealPreCommit2(sector, pre_commit_1_output);
    };
    wapi->SealCommit1 = [&](const SectorId &sector,
                            const SealRandomness &ticket,
                            const InteractiveRandomness &seed,
                            std::vector<PieceInfo> pieces,
                            const SectorCids &cids) {
      return worker->sealCommit1(sector, ticket, seed, pieces, cids);
    };
    wapi->SealCommit2 = [&](const SectorId &sector,
                            const Commit1Output &commit_1_output) {
      return worker->sealCommit2(sector, commit_1_output);
    };
    wapi->FinalizeSector = [&](const SectorId &sector,
                               std::vector<Range> keep_unsealed) {
      return worker->finalizeSector(sector, keep_unsealed);
    };
    wapi->Remove = [&](const SectorId &sector) {
      return worker->remove(sector);
    };

    auto wrpc{api::makeRpc(*wapi)};
    auto wroutes{std::make_shared<api::Routes>()};

    wroutes->insert({"/remote", sector_storage::serveHttp(local_store)});

    api::serve(wrpc, wroutes, *io, "127.0.0.1", config.api_port);
    api::rpc::saveInfo(config.repo_path, config.api_port, "stub");

    IoThread thread;
    boost::asio::post(*(thread.io), [&] {
      spdlog::info("fuhon worker are registering");
      auto address{fmt::format("/ip4/127.0.0.1/tcp/{}/http", config.api_port)};
      auto maybe_error =
          mapi->WorkerConnect(address);  // TODO: make reconnect if failed

      if (maybe_error.has_error()) {
        spdlog::error("worker register error: {}",
                      maybe_error.error().message());
        return;
      }
      spdlog::info("fuhon worker registered");
    });

    spdlog::info("fuhon worker started");
    io->run();
    return outcome::success();
  }
}  // namespace fc

int main(int argc, char **argv) {
  OUTCOME_EXCEPT(config, fc::readConfig(argc, argv));
  OUTCOME_EXCEPT(fc::main(config));
}
