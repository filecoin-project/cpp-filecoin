/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"

namespace fc::storage::keystore {
  InMemoryKeyStore::InMemoryKeyStore(
      std::shared_ptr<BlsProvider> blsProvider,
      std::shared_ptr<Secp256k1ProviderDefault> secp256K1Provider)
      : KeyStore(std::move(blsProvider), std::move(secp256K1Provider)) {}

  outcome::result<bool> InMemoryKeyStore::has(const Address &address) const
      noexcept {
    return storage_.find(address) != storage_.end();
  }

  outcome::result<void> InMemoryKeyStore::put(
      Address address, typename KeyStore::TPrivateKey key) noexcept {
    OUTCOME_TRY(valid, checkAddress(address, key));
    if (!valid) return KeyStoreError::kWrongAddress;
    auto res = storage_.try_emplace(address, key);
    if (!res.second) return KeyStoreError::kAlreadyExists;

    return outcome::success();
  }

  outcome::result<void> InMemoryKeyStore::remove(
      const Address &address) noexcept {
    OUTCOME_TRY(found, has(address));
    if (!found) return KeyStoreError::kNotFound;
    storage_.erase(address);
    return outcome::success();
  }

  outcome::result<std::vector<Address>> InMemoryKeyStore::list() const
      noexcept {
    std::vector<Address> res;
    for (auto &it : storage_) {
      res.push_back(it.first);
    }
    return std::move(res);
  }

  outcome::result<typename KeyStore::TPrivateKey> InMemoryKeyStore::get(
      const Address &address) const noexcept {
    OUTCOME_TRY(found, has(address));
    if (!found) return KeyStoreError::kNotFound;
    return storage_.at(address);
  }

  const std::shared_ptr<KeyStore> kDefaultKeystore{
      std::make_shared<InMemoryKeyStore>(
          std::make_shared<crypto::bls::BlsProviderImpl>(),
          std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>())};
}  // namespace fc::storage::keystore
