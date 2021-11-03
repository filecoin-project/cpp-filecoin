/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>

#include "primitives/cid/cid.hpp"
#include "storage/buffer_map.hpp"

namespace fc::markets::storage::client::import_manager {
  using ::fc::storage::PersistentBufferMap;

  /**
   * Stores import information
   */
  struct Import {
    /// Storage identifier in multistore (not used in Fuhon yet)
    uint64_t store_id{};
    std::string error;
    CID root;
    std::string source;
    std::string path;
  };

  /**
   * Manages imported data files
   */
  class ImportManager {
   public:
    ImportManager(std::shared_ptr<PersistentBufferMap> imports_storage,
                  boost::filesystem::path imports_dir);

    /**
     * Imports data for deal
     * If file is not a CAR file, then methods creates CAR file from imported
     * file. Imported CAR files are stored into import directory where CAR
     * filename is root CID.
     *
     * @param path - path to file with data
     * @param is_car - is it a car? If yes, it should contain single root,
     * otherwise method returns error
     * @return root cid of imported data
     */
    outcome::result<CID> import(const boost::filesystem::path &path,
                                bool is_car);

    /**
     * Lists imported files
     */
    outcome::result<std::vector<Import>> list() const;

    /**
     * Return imported CAR file path
     * @param root - root CID
     * @return path to CAR file
     */
    outcome::result<boost::filesystem::path> get(const CID &root) const;

   private:
    outcome::result<boost::filesystem::path> makeFilename(
        const CID &root) const;

    outcome::result<void> addImported(const CID &root,
                                      const boost::filesystem::path &path);

    std::shared_ptr<PersistentBufferMap> imported_;
    boost::filesystem::path imports_dir_;
  };

}  // namespace fc::markets::storage::client::import_manager
