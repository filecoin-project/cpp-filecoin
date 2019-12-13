/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address.hpp"

#include <gtest/gtest.h>

using fc::primitives::Address;
using fc::primitives::Network;
using fc::primitives::Protocol;
using fc::primitives::Secp256k1PublicKeyHash;

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
