/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "in_memory_keystore.hpp"

#include <boost/foreach.hpp>

using fc::primitives::Address;
using fc::storage::keystore::InMemoryKeyStore;
using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

InMemoryKeyStore::InMemoryKeyStore(
    std::shared_ptr<BlsProvider> blsProvider,
    std::shared_ptr<Secp256k1Provider> secp256K1Provider)
    : KeyStore(std::move(blsProvider), std::move(secp256K1Provider)) {}

fc::outcome::result<bool> InMemoryKeyStore::Has(
    const Address &address) noexcept {
  try {
    return storage_.find(address) != storage_.end();
  } catch (std::exception &) {
    return KeyStoreError::UNKNOWN;
  }
}

fc::outcome::result<void> InMemoryKeyStore::Put(
    Address address, typename KeyStore::TPrivateKey key) noexcept {
  try {
    OUTCOME_TRY(valid, CheckAddress(address, key));
    if (!valid) return KeyStoreError::WRONG_ADDRESS;
    auto res = storage_.emplace(address, key);
    if (!res.second) return KeyStoreError::ALREADY_EXISTS;
  } catch (std::exception &) {
    return KeyStoreError::UNKNOWN;
  }
  return fc::outcome::success();
}

fc::outcome::result<void> InMemoryKeyStore::Remove(
    const Address &address) noexcept {
  try {
    OUTCOME_TRY(found, Has(address));
    if (!found) return KeyStoreError::NOT_FOUND;
    storage_.erase(address);
  } catch (std::exception &) {
    return KeyStoreError::UNKNOWN;
  }
  return fc::outcome::success();
}

fc::outcome::result<std::vector<Address>> InMemoryKeyStore::List() noexcept {
  std::vector<Address> res;
  for (auto &it : storage_) {
    res.push_back(it.first);
  }
  return std::move(res);
}

fc::outcome::result<typename KeyStore::TPrivateKey> InMemoryKeyStore::Get(
    const Address &address) noexcept {
  OUTCOME_TRY(found, Has(address));
  if (!found) return KeyStoreError::NOT_FOUND;
  return storage_[address];
}
