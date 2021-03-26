/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/import_manager/import_manager.hpp"

#include "common/error_text.hpp"
#include "common/file.hpp"
#include "storage/car/car.hpp"
#include "storage/unixfs/unixfs.hpp"

namespace fc::markets::storage::client::import_manager {
  using common::Buffer;
  using common::readFile;
  using ::fc::storage::car::loadCar;
  using ::fc::storage::unixfs::wrapFile;

  ImportManager::ImportManager(
      std::shared_ptr<PersistentBufferMap> imports_storage, IpldPtr ipld)
      : imported_{std::move(imports_storage)}, ipld_{std::move(ipld)} {}

  outcome::result<CID> ImportManager::import(
      const boost::filesystem::path &path, bool is_car) {
    CID root;
    if (is_car) {
      OUTCOME_TRY(cids, loadCar(*ipld_, path));
      if (cids.size() != 1) {
        return ERROR_TEXT(
            "StorageMarketImportManager: cannot import car with more that one "
            "root");
      }
      root = cids.front();
    } else {
      OUTCOME_TRY(data, readFile(path));
      OUTCOME_TRYA(root, wrapFile(*ipld_, data));
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
      std::string path((char *)cursor->value().data(), cursor->value().size());
      result.emplace_back(Import{0, "", root, "import", path});
      cursor->next();
    }
    return result;
  }

  outcome::result<void> ImportManager::addImported(
      const CID &root, const boost::filesystem::path &path) {
    OUTCOME_TRY(key, root.toBytes());
    return imported_->put(Buffer{key},
                          Buffer{common::span::cbytes(path.string())});
  }

}  // namespace fc::markets::storage::client::import_manager
