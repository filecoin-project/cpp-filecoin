/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/import_manager/import_manager.hpp"

#include "common/error_text.hpp"
#include "common/file.hpp"
#include "proofs/proof_engine.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/unixfs/unixfs.hpp"

namespace fc::markets::storage::client::import_manager {
  using common::Buffer;
  using common::readFile;
  using common::writeFile;
  using ::fc::storage::car::loadCar;
  using ::fc::storage::car::makeCar;
  using ::fc::storage::ipfs::InMemoryDatastore;
  using ::fc::storage::unixfs::wrapFile;
  using proofs::padPiece;

  ImportManager::ImportManager(
      std::shared_ptr<PersistentBufferMap> imports_storage,
      const boost::filesystem::path &imports_dir)
      : imported_{std::move(imports_storage)}, imports_dir_{imports_dir} {
    boost::filesystem::create_directories(imports_dir_);
  }

  outcome::result<CID> ImportManager::import(
      const boost::filesystem::path &path, bool is_car) {
    InMemoryDatastore ipld;
    CID root;
    if (is_car) {
      // validate CAR
      OUTCOME_TRY(cids, loadCar(ipld, path));
      if (cids.size() != 1) {
        return ERROR_TEXT(
            "StorageMarketImportManager: cannot import car with more that one "
            "root");
      }
      root = cids.front();
      OUTCOME_TRY(car_path, makeFilename(root));
      boost::filesystem::copy_file(
          path, car_path, boost::filesystem::copy_option::overwrite_if_exists);
      padPiece(car_path);
    } else {
      // make CAR
      OUTCOME_TRY(data, readFile(path));
      OUTCOME_TRYA(root, wrapFile(ipld, data));
      OUTCOME_TRY(car_data, makeCar(ipld, {root}));
      OUTCOME_TRY(car_path, makeFilename(root));
      OUTCOME_TRY(writeFile(car_path, car_data));
      padPiece(car_path);
    }
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
    return imported_->put(Buffer{key},
                          Buffer{common::span::cbytes(path.string())});
  }

}  // namespace fc::markets::storage::client::import_manager
