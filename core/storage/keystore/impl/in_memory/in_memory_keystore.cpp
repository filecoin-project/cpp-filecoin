/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/keystore/impl/in_memory/in_memory_keystore.hpp"

#include <boost/foreach.hpp>

using fc::primitives::address::Address;
using fc::storage::keystore::InMemoryKeyStore;
using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

InMemoryKeyStore::InMemoryKeyStore(
    std::shared_ptr<BlsProvider> blsProvider,
    std::shared_ptr<Secp256k1Provider> secp256K1Provider)
    : KeyStore(std::move(blsProvider), std::move(secp256K1Provider)) {}

fc::outcome::result<bool> InMemoryKeyStore::has(const Address &address) const
    noexcept {
  return storage_.find(address) != storage_.end();
}

fc::outcome::result<void> InMemoryKeyStore::put(
    Address address, typename KeyStore::TPrivateKey key) noexcept {
  OUTCOME_TRY(valid, checkAddress(address, key));
  if (!valid) return KeyStoreError::WRONG_ADDRESS;
  auto res = storage_.try_emplace(address, key);
  if (!res.second) return KeyStoreError::ALREADY_EXISTS;

  return fc::outcome::success();
}

fc::outcome::result<void> InMemoryKeyStore::remove(
    const Address &address) noexcept {
  OUTCOME_TRY(found, has(address));
  if (!found) return KeyStoreError::NOT_FOUND;
  storage_.erase(address);
  return fc::outcome::success();
}

fc::outcome::result<std::vector<Address>> InMemoryKeyStore::list() const
    noexcept {
  std::vector<Address> res;
  for (auto &it : storage_) {
    res.push_back(it.first);
  }
  return std::move(res);
}

fc::outcome::result<typename KeyStore::TPrivateKey> InMemoryKeyStore::get(
    const Address &address) const noexcept {
  OUTCOME_TRY(found, has(address));
  if (!found) return KeyStoreError::NOT_FOUND;
  return storage_.at(address);
}
