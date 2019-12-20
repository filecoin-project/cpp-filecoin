/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_IMPL_IN_MEMORY_REPOSITORY_HPP
#define FILECOIN_CORE_STORAGE_IMPL_IN_MEMORY_REPOSITORY_HPP

#include "storage/repository/repository.hpp"

namespace fc::storage::repository {

  using Version = Repository::Version;

  static constexpr Version kInMemoryRepositoryVersion = 1;

  /**
   * @brief In-memory implementation of Repository
   */
  class InMemoryRepository : public Repository {
   public:
    InMemoryRepository();

    ~InMemoryRepository() override = default;

    /**
     * Create Repository in-memory
     * @param config_path - path to config file
     * @return InMemoryRepository
     */
    static fc::outcome::result<std::shared_ptr<Repository>> create(
        const std::string &config_path);

    /** @copydoc Repository::getVersion() */
    outcome::result<Version> getVersion() const override;
  };

}  // namespace fc::storage::repository

#endif  // FILECOIN_CORE_STORAGE_IMPL_IN_MEMORY_REPOSITORY_HPP
