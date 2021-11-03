/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/import_manager/import_manager.hpp"

#include "common/error_text.hpp"
#include "common/file.hpp"
#include "proofs/proof_engine.hpp"
#include "storage/car/car.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/unixfs/unixfs.hpp"

namespace fc::markets::storage::client::import_manager {
  using common::readFile;
  using common::writeFile;
  using ::fc::storage::car::makeCar;
  using ::fc::storage::unixfs::wrapFile;
  using proofs::padPiece;

  ImportManager::ImportManager(
      std::shared_ptr<PersistentBufferMap> imports_storage,
      boost::filesystem::path imports_dir)
      : imported_{std::move(imports_storage)},
        imports_dir_{std::move(imports_dir)} {
    boost::filesystem::create_directories(imports_dir_);
  }

  outcome::result<CID> ImportManager::import(
      const boost::filesystem::path &path, bool is_car) {
    CID root;
    std::shared_ptr<MemoryIndexedCar> ipld;
    std::string tmp_path;
    if (is_car) {
      OUTCOME_TRYA(ipld, MemoryIndexedCar::make(path.string(), false));
      if (ipld->roots.size() != 1) {
        return ERROR_TEXT(
            "StorageMarketImportManager: cannot import car with more that one "
            "root");
      }
      root = ipld->roots.front();
    } else {
      tmp_path = path.string() + ".unixfs-tmp.car";
      OUTCOME_TRYA(ipld, MemoryIndexedCar::make(tmp_path, true));
      std::ifstream file{path.string()};
      OUTCOME_TRYA(root, wrapFile(*ipld, file));
    }
    OUTCOME_TRY(car_path, makeFilename(root));
    // add header with root and reorder objects
    OUTCOME_TRY(::fc::storage::car::makeSelectiveCar(
        *ipld, {{root, {}}}, car_path.string()));
    if (!tmp_path.empty()) {
      boost::filesystem::remove(tmp_path);
    }
    padPiece(car_path);
    OUTCOME_TRY(addImported(root, path));
    return root;
  }

  outcome::result<std::vector<Import>> ImportManager::list() const {
    auto cursor = imported_->cursor();
    cursor->seekToFirst();
    std::vector<Import> result;
    while (cursor->isValid()) {
      OUTCOME_TRY(root, CID::fromBytes(cursor->key()));
      std::string path{common::span::bytestr(cursor->value())};
      result.emplace_back(Import{0, "", root, "import", path});
      cursor->next();
    }
    return result;
  }

  outcome::result<boost::filesystem::path> ImportManager::get(
      const CID &root) const {
    OUTCOME_TRY(filename, makeFilename(root));
    if (!boost::filesystem::exists(filename)) {
      return ERROR_TEXT("StorageMarketClient ImportManager: File not found");
    }
    return std::move(filename);
  }

  outcome::result<boost::filesystem::path> ImportManager::makeFilename(
      const CID &root) const {
    OUTCOME_TRY(filename, root.toString());
    return imports_dir_ / filename;
  }

  outcome::result<void> ImportManager::addImported(
      const CID &root, const boost::filesystem::path &path) {
    OUTCOME_TRY(key, root.toBytes());
    return imported_->put(key, common::span::cbytes(path.string()));
  }

}  // namespace fc::markets::storage::client::import_manager
