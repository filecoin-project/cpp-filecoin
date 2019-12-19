/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/repository.hpp"

using fc::storage::ipfs::IpfsDatastore;
using fc::storage::keystore::KeyStore;
using fc::storage::repository::Repository;

Repository::Repository(std::shared_ptr<IpfsDatastore> ipldStore,
                       std::shared_ptr<KeyStore> keystore)
    : ipld_store_(std::move(ipldStore)), keystore_(std::move(keystore)) {}

std::shared_ptr<IpfsDatastore> Repository::getIpldStore() const noexcept {
  return ipld_store_;
}

std::shared_ptr<KeyStore> Repository::getKeyStore() const noexcept {
  return keystore_;
}
