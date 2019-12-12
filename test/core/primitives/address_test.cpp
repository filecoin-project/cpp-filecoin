/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address.hpp"

#include <gtest/gtest.h>

#include <libp2p/crypto/secp256k1_provider/secp256k1_provider_impl.hpp>

#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls_provider/impl/bls_provider_impl.hpp"
#include "testutil/outcome.hpp"

using fc::common::Blob;
using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::primitives::Address;
using fc::primitives::BLSPublicKeyHash;
using fc::primitives::Network;
using fc::primitives::Protocol;
using fc::primitives::Secp256k1PublicKeyHash;
using libp2p::crypto::secp256k1::Secp256k1Provider;
using libp2p::crypto::secp256k1::Secp256k1ProviderImpl;

struct AddressTest : public testing::Test {
  Address addrID_0{Network::MAINNET, 3232104785};
  Address addrID_1{Network::TESTNET, 3232104784};
  Address addrSecp256k1_0{
      Network::MAINNET,
      Secp256k1PublicKeyHash(std::array<uint8_t, 20>{
          0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
          0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28})};
  Address addrSecp256k1_1{
      Network::MAINNET,
      Secp256k1PublicKeyHash(std::array<uint8_t, 20>{
          0xB8, 0x82, 0x61, 0x9D, 0x46, 0x55, 0x8F, 0x3D, 0x9E, 0x31,
          0x6D, 0x11, 0xB4, 0x8D, 0xCF, 0x21, 0x13, 0x27, 0x02, 0x6A})};
  Address addrSecp256k1_2{
      Network::MAINNET,
      Secp256k1PublicKeyHash(std::array<uint8_t, 20>{
          0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC, 0xB9,
          0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28})};
  Address addrBls{
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
 * @given An ID Address
 * @when Calling its isKeyType() method
 * @then Return value is false
 */
TEST_F(AddressTest, IsNotKeyType) {
  EXPECT_FALSE(this->addrID_0.isKeyType());
}

/**
 * @given An Secp256k1 Address
 * @when Calling its isKeyType() method
 * @then Return value is true
 */
TEST_F(AddressTest, IsKeyType) {
  EXPECT_TRUE(this->addrSecp256k1_0.isKeyType());
}

/**
 * @given An Address
 * @when Checking address being equal to itself
 * @then Return value is true
 */
TEST_F(AddressTest, EqualSelfTrue) {
  ASSERT_TRUE(this->addrID_0 == this->addrID_0);
}

/**
 * @given Two different addresses
 * @when Checking the addresses not being equal
 * @then Return value is false
 */
TEST_F(AddressTest, EqualFalse) {
  ASSERT_FALSE(this->addrSecp256k1_0 == this->addrSecp256k1_1);
}

/**
 * @given Two equal addresses
 * @when Checking the addresses being equal
 * @then The addresses are equal
 */
TEST_F(AddressTest, AddressesNotEqual1) {
  ASSERT_TRUE(this->addrSecp256k1_0 == this->addrSecp256k1_2);
}

/**
 * @given Two addresses; address 1 belongs to mainnet, address 2 - to testnet
 * @when Checking if address1 is less than the address2
 * @then The statement holds
 */
TEST_F(AddressTest, AddressesLess1) {
  EXPECT_TRUE(this->addrSecp256k1_1 < this->addrID_1);
  EXPECT_FALSE(this->addrSecp256k1_1 == this->addrID_1);
  EXPECT_FALSE(this->addrID_1 < this->addrSecp256k1_1);
}

/**
 * @given Two addresses in one network but of different protocols
 * @when Checking if address with smaller protocol value is less than the other
 * one
 * @then The statement holds
 */
TEST_F(AddressTest, AddressesLess2) {
  EXPECT_TRUE(this->addrID_0 < this->addrSecp256k1_0);
  EXPECT_FALSE(this->addrID_0 == this->addrSecp256k1_0);
  EXPECT_FALSE(this->addrSecp256k1_0 < this->addrID_0);
}

/**
 * @given Two addresses
 * @when Checking if address1 is less than the address2
 * @then The statement doesn't hold
 */
TEST_F(AddressTest, AddressesLess3) {
  EXPECT_FALSE(this->addrSecp256k1_0 < this->addrSecp256k1_1);
  EXPECT_FALSE(this->addrSecp256k1_0 == this->addrSecp256k1_1);
  EXPECT_TRUE(this->addrSecp256k1_1 < this->addrSecp256k1_0);
}

/**
 * @given An address
 * @when Checking if it is less that itself
 * @then The statement doesnt hold
 */
TEST_F(AddressTest, AddressesLessSelf) {
  ASSERT_FALSE(this->addrSecp256k1_2 < this->addrSecp256k1_2);
}

/**
 * @given an Id address
 * @when verifySyntax is called()
 * @then true returned
 */
TEST_F(AddressTest, EmptyVerifyIdAddress) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, addrID_0.verifySyntax(empty));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressTest, EmptyVerifySecp256k1Address) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, addrSecp256k1_0.verifySyntax(empty));
  ASSERT_FALSE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressTest, VerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(hash,
                      fc::crypto::blake2b::blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
}

/**
 * @given an Secp256k1 address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressTest, NotVerifySecp256k1Address) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(hash,
                      fc::crypto::blake2b::blake2b_160(keypair.public_key));
  Address address{Network::MAINNET, Secp256k1PublicKeyHash{hash}};
  EXPECT_OUTCOME_TRUE(wrong_keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong data
 * @then false returned
 */
TEST_F(AddressTest, EmptyVerifyBlsAddress) {
  std::vector<uint8_t> empty{};
  EXPECT_OUTCOME_TRUE(res, addrBls.verifySyntax(empty));
  ASSERT_FALSE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with correct data
 * @then true returned
 */
TEST_F(AddressTest, VerifySecpBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  Address address{Network::MAINNET, BLSPublicKeyHash{keypair.public_key}};
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
}

/**
 * @given an BLS address
 * @when verifySyntax is called() with wrong public key
 * @then false returned
 */
TEST_F(AddressTest, NotVerifyBlsAddress) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  Address address{Network::MAINNET, BLSPublicKeyHash{keypair.public_key}};
  EXPECT_OUTCOME_TRUE(wrong_keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(wrong_keypair.public_key));
  ASSERT_FALSE(res);
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with testnet
 * @then correct address returned
 */
TEST_F(AddressTest, GenerateSecp256k1AddressTestnet) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(address, Address::makeFromSecp256k1PublicKey(Network::TESTNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::TESTNET, address.network);
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}

/**
 * @given an Secp256k1 public key
 * @when generate address called with mainnet
 * @then correct address returned
 */
TEST_F(AddressTest, GenerateSecp256k1AddressMainnet) {
  EXPECT_OUTCOME_TRUE(keypair, secp256k1_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(address, Address::makeFromSecp256k1PublicKey(Network::MAINNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::MAINNET, address.network);
  ASSERT_EQ(Protocol::SECP256K1, address.getProtocol());
}

/**
 * @given an BLS public key
 * @when generate address called with testnet
 * @then correct address returned
 */
TEST_F(AddressTest, GenerateBlsAddressTestnet) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(address, Address::makeFromBlsPublicKey(Network::TESTNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::TESTNET, address.network);
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}

/**
 * @given an BLS public key
 * @when generate address called with mainnet
 * @then correct address returned
 */
TEST_F(AddressTest, GenerateBlsAddressMainnet) {
  EXPECT_OUTCOME_TRUE(keypair, bls_provider->generateKeyPair());
  EXPECT_OUTCOME_TRUE(address, Address::makeFromBlsPublicKey(Network::MAINNET, keypair.public_key));
  ASSERT_TRUE(address.isKeyType());
  EXPECT_OUTCOME_TRUE(res, address.verifySyntax(keypair.public_key));
  ASSERT_TRUE(res);
  ASSERT_EQ(Network::MAINNET, address.network);
  ASSERT_EQ(Protocol::BLS, address.getProtocol());
}
