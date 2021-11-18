/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/address.hpp"
#include "testutil/outcome.hpp"

using fc::crypto::blake2b::blake2b_160;
using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::BlsProviderImpl;
using fc::crypto::secp256k1::Secp256k1ProviderDefault;
using fc::crypto::secp256k1::Secp256k1Sha256ProviderImpl;
using fc::primitives::address::Address;
using fc::primitives::address::BLSPublicKeyHash;
using fc::primitives::address::Network;
using fc::primitives::address::Protocol;
using fc::primitives::address::Secp256k1PublicKeyHash;

auto address_id{Address::makeFromId(3232104785)};
auto address_secp256k1{Address::makeSecp256k1(
    {0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
     0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28})};
auto address_bls{Address::makeBls(
    {0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9, 0x9A, 0x83,
     0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28, 0xB8, 0x82, 0x61, 0x9D,
     0x46, 0x55, 0x8F, 0x3D, 0x9E, 0x31, 0x6D, 0x11, 0xB4, 0x8D, 0xCF, 0x21,
     0x13, 0x27, 0x02, 0x6A, 0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A})};

struct AddressVerifierTest : public testing::Test {
  std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider{
      std::make_shared<Secp256k1Sha256ProviderImpl>()};
  std::shared_ptr<BlsProvider> bls_provider{
      std::make_shared<BlsProviderImpl>()};
};

/**
 * @given an Id address
 * @when verifySyntax is called()
 * @then true returned
 */
TEST_F(AddressVerifierTest, EmptyVerifyIdAddress) {
  std::vector<uint8_t> empty{};
  ASSERT_TRUE(address_id.verifySyntax(empty));
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifySecp256k1Address) {
  std::vector<uint8_t> empty{};
  ASSERT_FALSE(address_secp256k1.verifySyntax(empty));
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressVerifierTest, VerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  auto address{Address::makeSecp256k1(keypair.public_key)};
  ASSERT_TRUE(address.verifySyntax(keypair.public_key));
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressVerifierTest, NotVerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  auto address{Address::makeSecp256k1(keypair.public_key)};
  EXPECT_OUTCOME_TRUE(wrong_keypair, secp256k1_provider->generate());
  ASSERT_FALSE(address.verifySyntax(wrong_keypair.public_key));
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifyBlsAddress) {
  std::vector<uint8_t> empty{};
  ASSERT_FALSE(address_bls.verifySyntax(empty));
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressVerifierTest, VerifySecpBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  auto address{Address::makeBls(keypair.public_key)};
  ASSERT_TRUE(address.verifySyntax(keypair.public_key));
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressVerifierTest, NotVerifyBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  auto address{Address::makeBls(keypair.public_key)};
  EXPECT_OUTCOME_TRUE(wrong_keypair, bls_provider->generateKeyPair());
  ASSERT_FALSE(address.verifySyntax(wrong_keypair.public_key));
}

/**
 * @given an BLS public key
 * @when generate address called with network
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  auto address = Address::makeBls(keypair.public_key);
  ASSERT_TRUE(address.isKeyType());
  ASSERT_TRUE(address.verifySyntax(keypair.public_key));
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with network
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateSecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  auto address = Address::makeSecp256k1(keypair.public_key);
  ASSERT_TRUE(address.isKeyType());
  ASSERT_TRUE(address.verifySyntax(keypair.public_key));
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}
