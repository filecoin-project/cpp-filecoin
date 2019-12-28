/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"

#include <gtest/gtest.h>

#include "crypto/blake2/blake2b.h"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "primitives/address/impl/address_verifier_impl.hpp"
#include "testutil/outcome.hpp"

using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::address::Address;
using fc::primitives::address::AddressBuilderImpl;
using fc::primitives::address::AddressVerifier;
using fc::primitives::address::AddressVerifierImpl;
using fc::primitives::address::Network;
using fc::storage::keystore::InMemoryKeyStore;
using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;
using libp2p::crypto::CryptoProviderError;
using libp2p::crypto::secp256k1::Secp256k1Provider;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;
using BlsKeyPair = fc::crypto::bls::KeyPair;
using Secp256k1KeyPair = libp2p::crypto::secp256k1::KeyPair;
using BlsSignature = fc::crypto::bls::Signature;
using Secp256k1Signature = libp2p::crypto::secp256k1::Signature;
using BlsPublicKey = fc::crypto::bls::PublicKey;
using Secp256k1PublicKey = libp2p::crypto::secp256k1::PublicKey;
using fc::primitives::address::decode;

class InMemoryKeyStoreTest : public ::testing::Test {
 public:
  std::shared_ptr<BlsProvider> bls_provider_;
  std::shared_ptr<BlsKeyPair> bls_keypair_;
  Address bls_address_{};

  std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  std::shared_ptr<Secp256k1KeyPair> secp256k1_keypair_;
  Address secp256k1_address_{};

  std::shared_ptr<AddressVerifier> address_verifier_;
  std::shared_ptr<KeyStore> ks;

  /** Some data to sign */
  std::vector<uint8_t> data_{1, 1, 2, 3, 5, 8, 13, 21};

  /**
   * Create crypto.
   */
  void SetUp() override {
    AddressBuilderImpl addressBuilder{};
    bls_provider_ = std::make_shared<BlsProviderImpl>();
    bls_keypair_ =
        std::make_shared<BlsKeyPair>(bls_provider_->generateKeyPair().value());

    bls_address_ =
        addressBuilder
            .makeFromBlsPublicKey(Network::MAINNET, bls_keypair_->public_key)
            .value();

    secp256k1_provider_ = std::make_shared<Secp256k1ProviderImpl>();
    secp256k1_keypair_ = std::make_shared<Secp256k1KeyPair>(
        secp256k1_provider_->generateKeyPair().value());

    secp256k1_address_ =
        addressBuilder
            .makeFromSecp256k1PublicKey(Network::MAINNET,
                                        secp256k1_keypair_->public_key)
            .value();

    address_verifier_ = std::make_shared<AddressVerifierImpl>();

    ks = std::make_shared<InMemoryKeyStore>(
        bls_provider_, secp256k1_provider_, address_verifier_);
  }

 protected:
  static bool VectorContains(const std::vector<Address> &vector,
                             Address value) {
    return std::find(vector.begin(), vector.end(), value) != vector.end();
  }

  bool checkSignature(gsl::span<uint8_t> message,
                      const BlsSignature &signature,
                      const BlsPublicKey &public_key) {
    auto res = bls_provider_->verifySignature(message, signature, public_key);
    if (res)
      return res.value();
    else
      return false;
  }

  bool checkSignature(gsl::span<uint8_t> message,
                      const Secp256k1Signature &signature,
                      const Secp256k1PublicKey &public_key) {
    auto res = secp256k1_provider_->verify(message, signature, public_key);
    if (res)
      return res.value();
    else
      return false;
  }
};

/**
 * @given Keystore is empty
 * @when Has() is called
 * @then success returned false
 */
TEST_F(InMemoryKeyStoreTest, HasEmpty) {
  EXPECT_OUTCOME_TRUE(found, ks->has(bls_address_));
  ASSERT_FALSE(found);
}

/**
 * @given Keystore, public key and address
 * @when try to insert key that already is in Keystore
 * @then ALREADY_EXISTS returned
 */
TEST_F(InMemoryKeyStoreTest, AddressAlreadyStored) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));
  EXPECT_OUTCOME_ERROR(KeyStoreError::ALREADY_EXISTS,
                       ks->put(bls_address_, bls_keypair_->private_key));
}

/**
 * @given Keystore, public key and address
 * @when try to remove key that is not in Keystore
 * @then NOT_FOUND returned
 */
TEST_F(InMemoryKeyStoreTest, RemoveNotExists) {
  EXPECT_OUTCOME_ERROR(KeyStoreError::NOT_FOUND, ks->remove(bls_address_));
}

/**
 * @given Keystore, public key and address
 * @when add key to Keystore and then delete it
 * @then success returned, key not found
 */
TEST_F(InMemoryKeyStoreTest, AddAndRemove) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));

  EXPECT_OUTCOME_TRUE(found, ks->has(bls_address_));
  ASSERT_TRUE(found);

  EXPECT_OUTCOME_TRUE_1(ks->remove(bls_address_));
  EXPECT_OUTCOME_TRUE(not_found, ks->has(bls_address_));
  ASSERT_FALSE(not_found);
}

/**
 * @given Keystore is empty
 * @when call list
 * @then empty list returned
 */
TEST_F(InMemoryKeyStoreTest, ListEmpty) {
  EXPECT_OUTCOME_TRUE(list, ks->list());
  ASSERT_EQ(0, list.size());
}

/**
 * @given Keystore stores 2 keys
 * @when call list
 * @then list containing all addresses returned
 */
TEST_F(InMemoryKeyStoreTest, ListKeys) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));
  EXPECT_OUTCOME_TRUE_1(
      ks->put(secp256k1_address_, secp256k1_keypair_->private_key));

  EXPECT_OUTCOME_TRUE(list, ks->list());
  ASSERT_EQ(2, list.size());
  ASSERT_TRUE(VectorContains(list, bls_address_));
  ASSERT_TRUE(VectorContains(list, secp256k1_address_));
}

/**
 * @given empty Keystore
 * @when sign with wrong address
 * @then NOT_FOUND returned
 */
TEST_F(InMemoryKeyStoreTest, SignNotFound) {
  EXPECT_OUTCOME_ERROR(KeyStoreError::NOT_FOUND, ks->sign(bls_address_, data_));
}

/**
 * @given empty Keystore
 * @when put malformed address
 * @then WRONG_ADDRESS returned
 */
TEST_F(InMemoryKeyStoreTest, SignWrongAddress) {
  // generate id address (type 0) which has no key
  std::vector<uint8_t> bytes{0x0, 0xD1, 0xC2, 0xA7, 0x0F};
  auto wrong_address = decode(bytes).value();
  EXPECT_OUTCOME_ERROR(KeyStoreError::WRONG_ADDRESS,
                       ks->put(wrong_address, bls_keypair_->private_key));
}

/**
 * @given Kestore with bls private key
 * @when Sign() called with bls crypto
 * @then correct signature returned
 */
TEST_F(InMemoryKeyStoreTest, SignCorrectBls) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));
  EXPECT_OUTCOME_TRUE(signature, ks->sign(bls_address_, data_));
  auto bls_signature = boost::get<BlsSignature>(signature);
  ASSERT_TRUE(checkSignature(data_, bls_signature, bls_keypair_->public_key));
}

/**
 * @given Kestore with secp256k1 private key
 * @when Sign() called with bls crypto
 * @then correct signature returned
 */
TEST_F(InMemoryKeyStoreTest, SignCorrectSecp256k1) {
  EXPECT_OUTCOME_TRUE_1(
      ks->put(secp256k1_address_, secp256k1_keypair_->private_key));
  EXPECT_OUTCOME_TRUE(signature, ks->sign(secp256k1_address_, data_));
  auto secp256k1_signature = boost::get<Secp256k1Signature>(signature);
  ASSERT_TRUE(checkSignature(
      data_, secp256k1_signature, secp256k1_keypair_->public_key));
}

/**
 * @given empty Keystore
 * @when Verify() with wrong address
 * @then NOT_FOUND returned
 */
TEST_F(InMemoryKeyStoreTest, VerifyNotFound) {
  BlsSignature signature;
  EXPECT_OUTCOME_ERROR(KeyStoreError::NOT_FOUND,
                       ks->verify(bls_address_, data_, signature));
}

/**
 * @given Keystore with bls private key
 * @when Verify() called with wrong signature
 * @then false returned
 */
TEST_F(InMemoryKeyStoreTest, VerifyWrongBls) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));
  BlsSignature signature;
  EXPECT_OUTCOME_TRUE(res, ks->verify(bls_address_, data_, signature));
  ASSERT_FALSE(res);
}

/**
 * @given Kestore with secp256k1 private key
 * @when Verify() called with invalid signature
 * @then false returned
 */
TEST_F(InMemoryKeyStoreTest, InvalidSecp256k1Signature) {
  EXPECT_OUTCOME_TRUE_1(
      ks->put(secp256k1_address_, secp256k1_keypair_->private_key));
  Secp256k1Signature invalid_signature;
  EXPECT_OUTCOME_ERROR(
      CryptoProviderError::SIGNATURE_VERIFICATION_FAILED,
      ks->verify(secp256k1_address_, data_, invalid_signature));
}

/**
 * @given Kestore with secp256k1 private key
 * @when Verify() called with wrong signature
 * @then false returned
 */
TEST_F(InMemoryKeyStoreTest, WrongSecp256k1Signature) {
  EXPECT_OUTCOME_TRUE_1(
      ks->put(secp256k1_address_, secp256k1_keypair_->private_key));

  EXPECT_OUTCOME_TRUE(other_keypair, secp256k1_provider_->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      wrong_signature,
      secp256k1_provider_->sign(data_, other_keypair.private_key));
  EXPECT_OUTCOME_TRUE(res,
                      ks->verify(secp256k1_address_, data_, wrong_signature));
  ASSERT_FALSE(res);
}

/**
 * @given Keystore with bls private key
 * @when Verify() called with correct signature
 * @then true returned
 */
TEST_F(InMemoryKeyStoreTest, VerifyCorrectBls) {
  EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_->private_key));
  EXPECT_OUTCOME_TRUE(signature,
                      bls_provider_->sign(data_, bls_keypair_->private_key));
  EXPECT_OUTCOME_TRUE(res, ks->verify(bls_address_, data_, signature));
  ASSERT_TRUE(res);
}
