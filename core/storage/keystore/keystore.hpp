/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_KEYSTORE_HPP
#define FILECOIN_CORE_STORAGE_KEYSTORE_HPP

#include <gsl/span>

#include <boost/variant.hpp>
#include "common/outcome.hpp"
#include "crypto/bls/bls_provider.hpp"
#include "crypto/bls/bls_types.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_verifier.hpp"
#include "storage/keystore/keystore_error.hpp"

namespace fc::storage::keystore {

  using crypto::bls::BlsProvider;
  using crypto::secp256k1::Secp256k1Provider;
  using BlsKeyPair = crypto::bls::KeyPair;
  using BlsPrivateKey = crypto::bls::PrivateKey;
  using BlsSignature = crypto::bls::Signature;
  using Secp256k1KeyPair = crypto::secp256k1::KeyPair;
  using Secp256k1PrivateKey = crypto::secp256k1::PrivateKey;
  using Secp256k1Signature = crypto::secp256k1::Signature;
  using crypto::signature::Signature;
  using primitives::address::Address;
  using primitives::address::AddressVerifier;

  /**
   * An interface to a facility to store and use cryptographic keys
   */
  class KeyStore {
   public:
    using TPrivateKey = boost::variant<BlsPrivateKey, Secp256k1PrivateKey>;

    KeyStore(std::shared_ptr<BlsProvider> blsProvider,
             std::shared_ptr<Secp256k1Provider> secp256K1Provider,
             std::shared_ptr<AddressVerifier> addressVerifier);

    virtual ~KeyStore() = default;

    /**
     * @brief Whether or not key exists in the Keystore
     * @param address key identifier
     */
    virtual outcome::result<bool> has(const Address &address) const
        noexcept = 0;

    /**
     * @brief stores a key in the Keystore
     * @param address - key idnetifier
     * @param key
     */
    virtual outcome::result<void> put(Address address,
                                      TPrivateKey key) noexcept = 0;

    /**
     * @brief remove key from keystore
     * @param address - key identifier
     */
    virtual outcome::result<void> remove(const Address &address) noexcept = 0;

    /**
     * @brief list key identifiers
     * @return list of addresses associated with keys
     */
    virtual outcome::result<std::vector<Address>> list() const noexcept = 0;

    /**
     * @brief Sign data with private key associated with address
     * @param address of private key stored
     * @param data to sign
     * @return signature
     */
    virtual outcome::result<Signature> sign(
        const Address &address, gsl::span<const uint8_t> data) noexcept;

    /**
     * @brief verify signature
     * @param address of keypair
     * @param data
     * @param signature
     */
    virtual outcome::result<bool> verify(const Address &address,
                                         gsl::span<const uint8_t> data,
                                         const Signature &signature) const
        noexcept;

   protected:
    /**
     * @brief Check address and key are valid
     * @param address
     * @param private_key
     * @return true if valid, false otherwise
     */
    virtual outcome::result<bool> checkAddress(
        const Address &address, const TPrivateKey &private_key) const noexcept;

    /**
     * @brief Get private key by address
     * @param address
     * @return private key
     */
    virtual outcome::result<TPrivateKey> get(const Address &address) const
        noexcept = 0;

   private:
    std::shared_ptr<BlsProvider> bls_provider_;
    std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<AddressVerifier> address_verifier_;
  };

}  // namespace fc::storage::keystore

#endif  // FILECOIN_CORE_STORAGE_KEYSTORE_HPP
