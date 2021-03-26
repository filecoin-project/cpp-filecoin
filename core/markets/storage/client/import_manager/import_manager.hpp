/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>

#include "storage/buffer_map.hpp"
#include "storage/ipfs/datastore.hpp"

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
                  IpldPtr ipld);

    /**
     * Imports data for deal.
     * @param path - path to file with data
     * @param is_car - is it a car? If yes, it shoud contain single root,
     * otherwise method returns error
     * @return root cid of imported data
     */
    outcome::result<CID> import(const boost::filesystem::path &path,
                                bool is_car);

    /**
     * Lists imported files
     */
    outcome::result<std::vector<Import>> list() const;

   private:
    outcome::result<void> addImported(const CID &root,
                                      const boost::filesystem::path &path);

    std::shared_ptr<PersistentBufferMap> imported_;
    IpldPtr ipld_;
  };

}  // namespace fc::markets::storage::client::import_manager
