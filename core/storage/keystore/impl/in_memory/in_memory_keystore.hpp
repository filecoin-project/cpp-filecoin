/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
                     std::shared_ptr<Secp256k1ProviderDefault> secp256K1Provider);

    ~InMemoryKeyStore() override = default;

    /** @copydoc KeyStore::has() */
    outcome::result<bool> has(const Address &address) const noexcept override;

    /** @copydoc KeyStore::put() */
    outcome::result<void> put(
        Address address, typename KeyStore::TPrivateKey key) noexcept override;

    /** @copydoc KeyStore::remove() */
    outcome::result<void> remove(const Address &address) noexcept override;

    /** @copydoc KeyStore::list() */
    outcome::result<std::vector<Address>> list() const noexcept override;

   protected:
    /** @copydoc KeyStore::get() */
    outcome::result<typename KeyStore::TPrivateKey> get(
        const Address &address) const noexcept override;

   private:
    TMapStorage storage_;
  };

}  // namespace fc::storage::keystore
