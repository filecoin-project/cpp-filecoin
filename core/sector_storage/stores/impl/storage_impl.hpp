/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem.hpp>

#include "sector_storage/stores/storage.hpp"

namespace fc::sector_storage::stores {
  struct LocalStorageImpl : LocalStorage {
    LocalStorageImpl(const boost::filesystem::path &path);

    outcome::result<FsStat> getStat(const std::string &path) const override;
    outcome::result<boost::optional<StorageConfig>> getStorage() const override;
    outcome::result<void> setStorage(
        std::function<void(StorageConfig &)> action) override;
    outcome::result<uint64_t> getDiskUsage(
        const std::string &path) const override;

      outcome::result<void> setApiToken(const std::string &token) override;

      outcome::result<std::string> getApiToken() const override;

      outcome::result<void> setSecret(const std::string &secret) override;

      outcome::result<std::string> getSecret() const override;

  private:
    boost::filesystem::path root_path_;
  };
}  // namespace fc::sector_storage::stores
