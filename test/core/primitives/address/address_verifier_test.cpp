/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/impl/address_verifier_impl.hpp"

#include <gtest/gtest.h>

#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_builder.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "testutil/outcome.hpp"

using fc::crypto::blake2b::blake2b_160;
using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::address::Address;
using fc::primitives::address::AddressBuilder;
using fc::primitives::address::AddressBuilderImpl;
using fc::primitives::address::AddressVerifier;
using fc::primitives::address::AddressVerifierImpl;
using fc::primitives::address::BLSPublicKeyHash;
using fc::primitives::address::Network;
using fc::primitives::address::Protocol;
using fc::primitives::address::Secp256k1PublicKeyHash;
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

 protected:
  std::shared_ptr<AddressVerifier> address_verifier =
      std::make_shared<AddressVerifierImpl>();
  std::shared_ptr<AddressBuilder> address_builder =
      std::make_shared<AddressBuilderImpl>();
};

/**
 * @given an Id address
 * @when verifySyntax is called()
 * @then true returned
 */
TEST_F(AddressVerifierTest, EmptyVerifyIdAddress) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, address_verifier->verifySyntax(address_id, empty));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifySecp256k1Address) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res,
                      address_verifier->verifySyntax(address_secp256k1, empty));
  ASSERT_FALSE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressVerifierTest, VerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  EXPECT_OUTCOME_TRUE(hash, blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressVerifierTest, NotVerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  EXPECT_OUTCOME_TRUE(hash, blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(wrong_keypair, secp256k1_provider->generate());
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressVerifierTest, EmptyVerifyBlsAddress) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, address_verifier->verifySyntax(address_bls, empty));
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
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, keypair.public_key));
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
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

class AddressVerifierParametrizedTest
    : public AddressVerifierTest,
      public testing::WithParamInterface<Network> {};

/**
 * @given an BLS public key
 * @when generate address called with network
 * @then correct address returned
 */
TEST_P(AddressVerifierParametrizedTest, GenerateBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(
      address,
      address_builder->makeFromBlsPublicKey(GetParam(), keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(GetParam(), address.network);
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with network
 * @then correct address returned
 */
TEST_P(AddressVerifierParametrizedTest, GenerateSecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generate());
  EXPECT_OUTCOME_TRUE(address,
                      address_builder->makeFromSecp256k1PublicKey(
                          GetParam(), keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(
      res, address_verifier->verifySyntax(address, keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(GetParam(), address.network);
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}

INSTANTIATE_TEST_CASE_P(InstatiateBlsNetTest,
                        AddressVerifierParametrizedTest,
                        testing::Values(Network::TESTNET, Network::MAINNET));
