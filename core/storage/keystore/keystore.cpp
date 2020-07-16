/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "keystore.hpp"

#include "common/visitor.hpp"

namespace fc::storage::keystore {

  using crypto::signature::Signature;
  using primitives::address::BLSPublicKeyHash;
  using primitives::address::Protocol;
  using primitives::address::Secp256k1PublicKeyHash;

  KeyStore::KeyStore(
      std::shared_ptr<BlsProvider> blsProvider,
      std::shared_ptr<Secp256k1ProviderDefault> secp256K1Provider)
      : bls_provider_(std::move(blsProvider)),
        secp256k1_provider_(std::move(secp256K1Provider)) {}

  fc::outcome::result<bool> KeyStore::checkAddress(
      const Address &address, const TPrivateKey &private_key) const noexcept {
    if (!address.isKeyType()) return false;

    static_assert(std::is_same_v<BlsPrivateKey, Secp256k1PrivateKey>);

    if (address.getProtocol() == Protocol::BLS) {
      OUTCOME_TRY(public_key,
                  bls_provider_->derivePublicKey(
                      boost::get<BlsPrivateKey>(private_key)));
      return address.verifySyntax(public_key);
    }
    if (address.getProtocol() == Protocol::SECP256K1) {
      OUTCOME_TRY(public_key,
                  secp256k1_provider_->derive(
                      boost::get<Secp256k1PrivateKey>(private_key)));
      return address.verifySyntax(public_key);
    }

    return KeyStoreError::kWrongAddress;
  }

  fc::outcome::result<Signature> KeyStore::sign(
      const Address &address, gsl::span<const uint8_t> data) noexcept {
    OUTCOME_TRY(private_key, get(address));
    OUTCOME_TRY(valid, checkAddress(address, private_key));
    if (!valid) return KeyStoreError::kWrongAddress;

    if (address.getProtocol() == Protocol::BLS) {
      OUTCOME_TRY(
          signature,
          bls_provider_->sign(data, boost::get<BlsPrivateKey>(private_key)));
      return signature;
    }
    if (address.getProtocol() == Protocol::SECP256K1) {
      OUTCOME_TRY(signature,
                  secp256k1_provider_->sign(
                      data, boost::get<Secp256k1PrivateKey>(private_key)));
      return std::move(signature);
    }

    return KeyStoreError::kWrongAddress;
  }

  fc::outcome::result<bool> KeyStore::verify(const Address &address,
                                             gsl::span<const uint8_t> data,
                                             const Signature &signature) const
      noexcept {
    return visit_in_place(
        signature,
        [this, address, data](
            const BlsSignature &bls_signature) -> fc::outcome::result<bool> {
          if (address.getProtocol() != Protocol::BLS
              || address.data.type() != typeid(BLSPublicKeyHash)) {
            return KeyStoreError::kWrongSignature;
          }

          BlsPublicKey public_key{boost::get<BLSPublicKeyHash>(address.data)};
          return bls_provider_->verifySignature(
              data, bls_signature, public_key);
        },
        [this, address, data](const Secp256k1Signature &secp256k1_signature)
            -> fc::outcome::result<bool> {
          if (address.getProtocol() != Protocol::SECP256K1
              || address.data.type() != typeid(Secp256k1PublicKeyHash)) {
            return KeyStoreError::kWrongSignature;
          }

          OUTCOME_TRY(
              public_key,
              secp256k1_provider_->recoverPublicKey(data, secp256k1_signature));
          return address.verifySyntax(public_key);
        });
  }

}  // namespace fc::storage::keystore
