/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/address_verifier.hpp"

#include <gtest/gtest.h>

#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>

#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls_provider/impl/bls_provider_impl.hpp"
#include "primitives/address.hpp"
#include "primitives/address/address_builder.hpp"
#include "testutil/outcome.hpp"

using fc::crypto::blake2b::blake2b_160;
using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::Address;
using fc::primitives::BLSPublicKeyHash;
using fc::primitives::Network;
using fc::primitives::Protocol;
using fc::primitives::Secp256k1PublicKeyHash;
using fc::primitives::address::makeFromBlsPublicKey;
using fc::primitives::address::makeFromSecp256k1PublicKey;
using fc::primitives::address::verifySyntax;
using libp2p::crypto::secp256k1::Secp256k1Provider;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;

struct AddressVerifierTest : public testing::Test {
  Address address_id{Network::MAINNET, 3232104785};
  Address address_secp256k1{
      Network::MAINNET,
      Secp256k1PublicKeyHash(std::array<uint8_t, 20>{
          0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
          0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28})};
  Address address_bls{
      Network::MAINNET,
      BLSPublicKeyHash(std::array<uint8_t, 48>{
          0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
          0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28,
          0xB8, 0x82, 0x61, 0x9D, 0x46, 0x55, 0x8F, 0x3D, 0x9E, 0x31,
          0x6D, 0x11, 0xB4, 0x8D, 0xCF, 0x21, 0x13, 0x27, 0x02, 0x6A,
          0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A})};
  std::shared_ptr<Secp256k1Provider> secp256k1_provider{
      std::make_shared<Secp256k1ProviderImpl>()};
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
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address_id, empty));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifySecp256k1Address) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address_secp256k1, empty));
  ASSERT_FALSE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressVerifierTest, VerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(hash, blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressVerifierTest, NotVerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(hash, blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(wrong_keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifyBlsAddress) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address_bls, empty));
  ASSERT_FALSE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressVerifierTest, VerifySecpBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  Address address{Network::MAINNET, BLSPublicKeyHash{keypair.public_key}};
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressVerifierTest, NotVerifyBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  Address address{Network::MAINNET, BLSPublicKeyHash{keypair.public_key}};
  EXPECT_OUTCOME_TRUE(wrong_keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with testnet
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateSecp256k1AddressTestnet) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      address,
      makeFromSecp256k1PublicKey(Network::TESTNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::TESTNET, address.network);
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with mainnet
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateSecp256k1AddressMainnet) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      address,
      makeFromSecp256k1PublicKey(Network::MAINNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::MAINNET, address.network);
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}

/**
 * @given an BLS public key
 * @when generate address called with testnet
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateBlsAddressTestnet) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      address, makeFromBlsPublicKey(Network::TESTNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::TESTNET, address.network);
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}

/**
 * @given an BLS public key
 * @when generate address called with mainnet
 * @then correct address returned
 */
TEST_F(AddressVerifierTest, GenerateBlsAddressMainnet) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      address, makeFromBlsPublicKey(Network::MAINNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::MAINNET, address.network);
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}
