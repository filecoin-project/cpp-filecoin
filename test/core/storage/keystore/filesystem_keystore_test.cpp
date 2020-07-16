/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/keystore/impl/filesystem/filesystem_keystore.hpp"

#include <gtest/gtest.h>
#include "crypto/blake2/blake2b.h"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_error.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/address_codec.hpp"
#include "testutil/outcome.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage::keystore {

  using fc::crypto::bls::BlsProvider;
  using fc::crypto::bls::BlsProviderImpl;
  using fc::crypto::secp256k1::Secp256k1Error;
  using fc::crypto::secp256k1::Secp256k1ProviderDefault;
  using fc::crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::primitives::address::Address;
  using fc::primitives::address::Network;
  using BlsKeyPair = fc::crypto::bls::KeyPair;
  using Secp256k1KeyPair = fc::crypto::secp256k1::KeyPair;
  using BlsSignature = fc::crypto::bls::Signature;
  using Secp256k1Signature = fc::crypto::secp256k1::Signature;
  using BlsPublicKey = fc::crypto::bls::PublicKey;
  using Secp256k1PublicKey = fc::crypto::secp256k1::PublicKey;
  using fc::primitives::address::decode;

  class FileSystemKeyStoreTest : public test::BaseFS_Test {
   public:
    FileSystemKeyStoreTest()
        : test::BaseFS_Test("fc_filesystem_keystore_test") {}

    std::shared_ptr<BlsProvider> bls_provider_;
    BlsKeyPair bls_keypair_;
    Address bls_address_{};

    std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider_;
    Secp256k1KeyPair secp256k1_keypair_;
    Address secp256k1_address_{};

    std::shared_ptr<KeyStore> ks;

    /** Some data to sign */
    std::vector<uint8_t> data_{1, 1, 2, 3, 5, 8, 13, 21};

    /**
     * Create crypto and directory.
     */
    void SetUp() override {
      BaseFS_Test::SetUp();

      bls_provider_ = std::make_shared<BlsProviderImpl>();
      bls_keypair_ = bls_provider_->generateKeyPair().value();

      bls_address_ = Address::makeBls(bls_keypair_.public_key);

      secp256k1_provider_ = std::make_shared<Secp256k1Sha256ProviderImpl>();
      secp256k1_keypair_ = secp256k1_provider_->generate().value();

      secp256k1_address_ =
          Address::makeSecp256k1(secp256k1_keypair_.public_key);

      ks = std::make_shared<FileSystemKeyStore>(
          base_path.string(), bls_provider_, secp256k1_provider_);
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
  TEST_F(FileSystemKeyStoreTest, HasEmpty) {
    EXPECT_OUTCOME_TRUE(found, ks->has(bls_address_));
    ASSERT_FALSE(found);
  }

  /**
   * @given Keystore, public key and address
   * @when try to insert key that already is in Keystore
   * @then ALREADY_EXISTS returned
   */
  TEST_F(FileSystemKeyStoreTest, AddressAlreadyStored) {
    EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_.private_key));
    EXPECT_OUTCOME_ERROR(KeyStoreError::kAlreadyExists,
                         ks->put(bls_address_, bls_keypair_.private_key));
  }

  /**
   * @given Keystore, public key and address
   * @when try to remove key that is not in Keystore
   * @then NOT_FOUND returned
   */
  TEST_F(FileSystemKeyStoreTest, RemoveNotExists) {
    EXPECT_OUTCOME_ERROR(KeyStoreError::kNotFound, ks->remove(bls_address_));
  }

  /**
   * @given Keystore, public key and address
   * @when add key to Keystore and then delete it
   * @then success returned, key not found
   */
  TEST_F(FileSystemKeyStoreTest, AddAndRemove) {
    EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_.private_key));

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
  TEST_F(FileSystemKeyStoreTest, ListEmpty) {
    EXPECT_OUTCOME_TRUE(list, ks->list());
    ASSERT_EQ(0, list.size());
  }

  /**
   * @given Keystore stores 2 keys
   * @when call list
   * @then list containing all addresses returned
   */
  TEST_F(FileSystemKeyStoreTest, ListKeys) {
    EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_.private_key));
    EXPECT_OUTCOME_TRUE_1(
        ks->put(secp256k1_address_, secp256k1_keypair_.private_key));

    EXPECT_OUTCOME_TRUE(list, ks->list());
    ASSERT_EQ(2, list.size());
    ASSERT_TRUE(VectorContains(list, bls_address_));
    ASSERT_TRUE(VectorContains(list, secp256k1_address_));
  }

  /**
   * @given empty Keystore
   * @when sign with wrong address
   * @then kNotFound returned
   */
  TEST_F(FileSystemKeyStoreTest, SignNotFound) {
    EXPECT_OUTCOME_ERROR(KeyStoreError::kNotFound,
                         ks->sign(bls_address_, data_));
  }

  /**
   * @given empty Keystore
   * @when put malformed address
   * @then WRONG_ADDRESS returned
   */
  TEST_F(FileSystemKeyStoreTest, SignWrongAddress) {
    // generate id address (type 0) which has no key
    std::vector<uint8_t> bytes{0x0, 0xD1, 0xC2, 0xA7, 0x0F};
    auto wrong_address = decode(bytes).value();
    EXPECT_OUTCOME_ERROR(KeyStoreError::kWrongAddress,
                         ks->put(wrong_address, bls_keypair_.private_key));
  }

  /**
   * @given Kestore with bls private key
   * @when Sign() called with bls crypto
   * @then correct signature returned
   */
  TEST_F(FileSystemKeyStoreTest, SignCorrectBls) {
    EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_.private_key));
    EXPECT_OUTCOME_TRUE(signature, ks->sign(bls_address_, data_));
    auto bls_signature = boost::get<BlsSignature>(signature);
    ASSERT_TRUE(checkSignature(data_, bls_signature, bls_keypair_.public_key));
  }

  /**
   * @given Kestore with secp256k1 private key
   * @when Sign() called with bls crypto
   * @then correct signature returned
   */
  TEST_F(FileSystemKeyStoreTest, SignCorrectSecp256k1) {
    EXPECT_OUTCOME_TRUE_1(
        ks->put(secp256k1_address_, secp256k1_keypair_.private_key));
    EXPECT_OUTCOME_TRUE(signature, ks->sign(secp256k1_address_, data_));
    auto secp256k1_signature = boost::get<Secp256k1Signature>(signature);
    ASSERT_TRUE(checkSignature(
        data_, secp256k1_signature, secp256k1_keypair_.public_key));
  }

  /**
   * @given empty Keystore
   * @when Verify() with wrong address
   * @then false returned
   */
  TEST_F(FileSystemKeyStoreTest, VerifyNotFound) {
    BlsSignature signature;
    EXPECT_OUTCOME_TRUE(res, ks->verify(bls_address_, data_, signature));
    ASSERT_FALSE(res);
  }

  /**
   * @given Keystore with bls private key
   * @when Verify() called with wrong signature
   * @then false returned
   */
  TEST_F(FileSystemKeyStoreTest, VerifyWrongBls) {
    EXPECT_OUTCOME_TRUE_1(ks->put(bls_address_, bls_keypair_.private_key));
    BlsSignature signature;
    EXPECT_OUTCOME_TRUE(res, ks->verify(bls_address_, data_, signature));
    ASSERT_FALSE(res);
  }

  /**
   * @given Kestore with secp256k1 private key
   * @when Verify() called with invalid signature
   * @then false returned
   */
  TEST_F(FileSystemKeyStoreTest, InvalidSecp256k1Signature) {
    EXPECT_OUTCOME_TRUE_1(
        ks->put(secp256k1_address_, secp256k1_keypair_.private_key));
    Secp256k1Signature invalid_signature;
    invalid_signature[64] = 99;
    EXPECT_OUTCOME_ERROR(
        Secp256k1Error::kSignatureParseError,
        ks->verify(secp256k1_address_, data_, invalid_signature));
  }

  /**
   * @given Kestore and secp256k1 signature and public key
   * @when Verify() called with signature
   * @then true returned
   */
  TEST_F(FileSystemKeyStoreTest, VerifySecp256k1Signature) {
    EXPECT_OUTCOME_TRUE(
        wrong_signature,
        secp256k1_provider_->sign(data_, secp256k1_keypair_.private_key));
    EXPECT_OUTCOME_TRUE(res,
                        ks->verify(secp256k1_address_, data_, wrong_signature));
    ASSERT_TRUE(res);
  }

  /**
   * @given Keystore and secp256k1 signature and wrong public key
   * @when Verify() called with wrong public ket
   * @then false returned
   */
  TEST_F(FileSystemKeyStoreTest, VerifyWrongSecp256k1Signature) {
    EXPECT_OUTCOME_TRUE(other_keypair, secp256k1_provider_->generate());
    Address other_address = Address::makeSecp256k1(other_keypair.public_key);

    EXPECT_OUTCOME_TRUE(
        secp256k1_signature,
        secp256k1_provider_->sign(data_, secp256k1_keypair_.private_key));
    EXPECT_OUTCOME_TRUE(res,
                        ks->verify(other_address, data_, secp256k1_signature));
    ASSERT_FALSE(res);
  }

  /**
   * @given Keystore with and bls signature and public key
   * @when Verify() called with signature
   * @then true returned
   */
  TEST_F(FileSystemKeyStoreTest, VerifyCorrectBls) {
    EXPECT_OUTCOME_TRUE(signature,
                        bls_provider_->sign(data_, bls_keypair_.private_key));
    EXPECT_OUTCOME_TRUE(res, ks->verify(bls_address_, data_, signature));
    ASSERT_TRUE(res);
  }

}  // namespace fc::storage::keystore
