/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/keystore/keystore.hpp"

#include "filecoin/common/visitor.hpp"

using fc::crypto::signature::Signature;
using fc::primitives::address::Protocol;
using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

KeyStore::KeyStore(std::shared_ptr<BlsProvider> blsProvider,
                   std::shared_ptr<Secp256k1Provider> secp256K1Provider)
    : bls_provider_(std::move(blsProvider)),
      secp256k1_provider_(std::move(secp256K1Provider)) {}

fc::outcome::result<bool> KeyStore::checkAddress(
    const Address &address, const TPrivateKey &private_key) const noexcept {
  if (!address.isKeyType()) return false;

  // TODO(a.chernyshov): use visit_in_place
  static_assert(std::is_same_v<BlsPrivateKey, Secp256k1PrivateKey>);

  if (address.getProtocol() == Protocol::BLS) {
    OUTCOME_TRY(
        public_key,
        bls_provider_->derivePublicKey(boost::get<BlsPrivateKey>(private_key)));
    return address.verifySyntax(public_key);
  }
  if (address.getProtocol() == Protocol::SECP256K1) {
    OUTCOME_TRY(public_key,
                secp256k1_provider_->derive(
                    boost::get<Secp256k1PrivateKey>(private_key)));
    return address.verifySyntax(public_key);
  }

  return KeyStoreError::WRONG_ADDRESS;
}

fc::outcome::result<Signature> KeyStore::sign(
    const Address &address, gsl::span<const uint8_t> data) noexcept {
  OUTCOME_TRY(private_key, get(address));
  OUTCOME_TRY(valid, checkAddress(address, private_key));
  if (!valid) return KeyStoreError::WRONG_ADDRESS;

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

  return KeyStoreError::WRONG_ADDRESS;
}

fc::outcome::result<bool> KeyStore::verify(const Address &address,
                                           gsl::span<const uint8_t> data,
                                           const Signature &signature) const
    noexcept {
  OUTCOME_TRY(private_key, get(address));
  OUTCOME_TRY(valid, checkAddress(address, private_key));
  if (!valid) return KeyStoreError::WRONG_ADDRESS;

  return visit_in_place(
      signature,
      [this, address, data, private_key](
          const BlsSignature &bls_signature) -> fc::outcome::result<bool> {
        if (address.getProtocol() != Protocol::BLS)
          return KeyStoreError::WRONG_SIGNATURE;
        OUTCOME_TRY(public_key,
                    bls_provider_->derivePublicKey(
                        boost::get<BlsPrivateKey>(private_key)));
        return bls_provider_->verifySignature(data, bls_signature, public_key);
      },
      [this, address, data, private_key](
          const Secp256k1Signature &secp256k1_signature)
          -> fc::outcome::result<bool> {
        if (address.getProtocol() != Protocol::SECP256K1)
          return KeyStoreError::WRONG_SIGNATURE;
        OUTCOME_TRY(public_key,
                    secp256k1_provider_->derive(
                        boost::get<Secp256k1PrivateKey>(private_key)));
        return secp256k1_provider_->verify(
            data, secp256k1_signature, public_key);
      });
}
