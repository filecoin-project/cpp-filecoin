/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/repository/impl/in_memory_repository.hpp"

#include "crypto/bls_provider/impl/bls_provider_impl.hpp"
#include "libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp"
#include "primitives/address/impl/address_verifier_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::address::AddressVerifierImpl;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::keystore::InMemoryKeyStore;
using fc::storage::repository::InMemoryRepository;
using fc::storage::repository::Repository;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;

InMemoryRepository::InMemoryRepository()
    : Repository(std::make_shared<InMemoryDatastore>(),
                 std::make_shared<InMemoryKeyStore>(
                     std::make_shared<BlsProviderImpl>(),
                     std::make_shared<Secp256k1ProviderImpl>(),
                     std::make_shared<AddressVerifierImpl>()),
                 std::make_shared<Config>()) {}

fc::outcome::result<std::shared_ptr<Repository>> InMemoryRepository::create(
    const std::string &config_path) {
  auto res = std::make_shared<InMemoryRepository>();
  OUTCOME_TRY(res->loadConfig(config_path));
  return res;
}

fc::outcome::result<unsigned int> InMemoryRepository::getVersion() const {
  return kInMemoryRepositoryVersion;
}
