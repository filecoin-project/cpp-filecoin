/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_IN_MEMORY_KEYSTORE_HPP
#define FILECOIN_CORE_STORAGE_IN_MEMORY_KEYSTORE_HPP

#include <map>

#include "storage/keystore/keystore.hpp"

namespace fc::storage::keystore {

  /**
   * @brief In-memory keystore implementation
   */
  class InMemoryKeyStore : public KeyStore {
    using TMapStorage = std::map<Address, typename KeyStore::TPrivateKey>;

   public:
    InMemoryKeyStore(std::shared_ptr<BlsProvider> blsProvider,
                     std::shared_ptr<Secp256k1Provider> secp256K1Provider,
                     std::shared_ptr<AddressVerifier> addressVerifier);

    ~InMemoryKeyStore() override = default;

    /** @copydoc KeyStore::Has() */
    outcome::result<bool> Has(const Address &address) noexcept override;

    /** @copydoc KeyStore::Put() */
    outcome::result<void> Put(
        Address address, typename KeyStore::TPrivateKey key) noexcept override;

    /** @copydoc KeyStore::Remove() */
    outcome::result<void> Remove(const Address &address) noexcept override;

    /** @copydoc KeyStore::List() */
    outcome::result<std::vector<Address>> List() noexcept override;

   protected:
    /** @copydoc KeyStore::Get() */
    outcome::result<typename KeyStore::TPrivateKey> Get(
        const Address &address) noexcept override;

   private:
    TMapStorage storage_;
  };

}  // namespace fc::storage::keystore

#endif  // FILECOIN_CORE_STORAGE_IN_MEMORY_KEYSTORE_HPP
