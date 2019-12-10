/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address_codec.hpp"

#include <algorithm>

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using fc::primitives::Address;
using fc::primitives::Network;
using fc::primitives::Protocol;

struct AddressCodecTest : public testing::Test {
  std::vector<uint8_t> randomNBytes(uint N) {
    std::vector<int> v(N);
    std::generate(v.begin(), v.end(), std::rand);
    std::vector<uint8_t> u(N);
    std::transform(v.begin(), v.end(), u.begin(), [](int k) -> uint8_t {
      return k % 256;
    });
    return u;
  }
};

// Address decoding from bytes
/**
 * @given A uint64_t number serialized as LEB128 byte sequence
 * @when Creating an ID Address for the given payload
 * @then ID Address successfully created
 */
TEST_F(AddressCodecTest, AddressDecodeOK) {
  std::vector<uint8_t> bytes{
      0x0,
      0x0,
      0xD1,
      0xC2,
      0xA7,
      0x0F};  // {network == MAINNET, protocol == ID, payload == 32104785}
  EXPECT_OUTCOME_TRUE_2(addr, fc::primitives::decode(bytes))
  EXPECT_EQ(boost::get<uint64_t>(addr.data), 32104785);
}

/**
 * @given A LEB128 encoding of some big number (greater than uint64_t can fit)
 * @when Creating an ID Address for the given payload
 * @then INVALID_PAYLOAD error occurs while decoding the payload into a uint64
 * integer
 */
TEST_F(AddressCodecTest, AddressDecodeFailure) {
  std::vector<uint8_t> bytes{0x1,
                             0x0,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x80,
                             0x01};  // 2^70
  int expected =
      static_cast<int>(fc::primitives::AddressError::INVALID_PAYLOAD);
  EXPECT_OUTCOME_FALSE_2(error_code, fc::primitives::decode(bytes));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given Array of 20 bytes reprsenting a blake2b-160 hash
 * @when Creating a Secp256k1 Address
 * @then Secp256k1 Address successfully created
 */
TEST_F(AddressCodecTest, AddressDecodeSecp256k1OK) {
  std::vector<uint8_t> payload = this->randomNBytes(20);
  std::vector<uint8_t> bytes{0x1, 0x1};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_TRUE_1(fc::primitives::decode(bytes));
}

/**
 * @given Array of 47 bytes reprsenting a BLS hash
 * @when Creating a BLS Address
 * @then INVALID_PAYLOAD error while creating the Address
 */
TEST_F(AddressCodecTest, InvalidPayloadSize) {
  std::vector<uint8_t> payload = this->randomNBytes(47);
  std::vector<uint8_t> bytes{0x1, 0x3};
  int expected =
      static_cast<int>(fc::primitives::AddressError::INVALID_PAYLOAD);
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_FALSE_2(error_code, fc::primitives::decode(bytes));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given An unsupported value as Address protocol
 * @when Creating an Address
 * @then Address is not created due to UNKNOWN_PROTOCOL error
 */
TEST_F(AddressCodecTest, UnknownProtocol) {
  std::vector<uint8_t> payload = this->randomNBytes(20);
  std::vector<uint8_t> bytes{0x1, 0x4};
  int expected =
      static_cast<int>(fc::primitives::AddressError::UNKNOWN_PROTOCOL);
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_FALSE_2(error_code, fc::primitives::decode(bytes));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given An ID Address in testnet
 * @when Calling its String() method
 * @then The output matches expected form "t0XXXXXXXX"
 */
TEST_F(AddressCodecTest, IDtoString) {
  std::vector<uint8_t> bytes{0x0, 0x0, 0xD1, 0xC2, 0xA7, 0x0F};  // 32104785
  EXPECT_OUTCOME_TRUE_2(addr, fc::primitives::decode(bytes))
  EXPECT_EQ(fc::primitives::encodeToString(addr), std::string{"f032104785"});
}

/**
 * @given A non-ID address
 * @when Check the size of its checksum
 * @then The size is exactly 4 bytes
 */
TEST_F(AddressCodecTest, ChecksumSize) {
  std::vector<uint8_t> payload = this->randomNBytes(20);
  std::vector<uint8_t> bytes{0x1, 0x1};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_TRUE_2(addr, fc::primitives::decode(bytes));
  ASSERT_TRUE(fc::primitives::checksum(addr).size() == 4);
}

// Verify known addresses serializations to string match expectation
// https://filecoin-project.github.io/specs/#id-type-addresses
/**
 * @given A set of pairs (address_in_hex, address_base32_string)
 * @when Serializing addresses to strings
 * @then The outputs match expected strings
 */
TEST_F(AddressCodecTest, EncodeToString) {
  EXPECT_OUTCOME_TRUE_2(
      addr,
      fc::primitives::decode(std::vector<uint8_t>{
          0x0,  0x1,  0xFD, 0x1D, 0x0F, 0x4D, 0xFC, 0xD7, 0xE9, 0x9A, 0xFC,
          0xB9, 0x9A, 0x83, 0x26, 0xB7, 0xDC, 0x45, 0x9D, 0x32, 0xC6, 0x28}));
  EXPECT_EQ(fc::primitives::encodeToString(addr),
            std::string{"f17uoq6tp427uzv7fztkbsnn64iwotfrristwpryy"});

  EXPECT_OUTCOME_TRUE_2(
      addr2,
      fc::primitives::decode(std::vector<uint8_t>{
          0x0,  0x1,  0xB8, 0x82, 0x61, 0x9D, 0x46, 0x55, 0x8F, 0x3D, 0x9E,
          0x31, 0x6D, 0x11, 0xB4, 0x8D, 0xCF, 0x21, 0x13, 0x27, 0x02, 0x6A}));
  EXPECT_EQ(fc::primitives::encodeToString(addr2),
            std::string{"f1xcbgdhkgkwht3hrrnui3jdopeejsoatkzmoltqy"});

  EXPECT_OUTCOME_TRUE_2(
      addr3,
      fc::primitives::decode(std::vector<uint8_t>{
          0x0,  0x2,  0x31, 0x6B, 0x4C, 0x1F, 0xF5, 0xD4, 0xAF, 0xB7, 0x82,
          0x6C, 0xEA, 0xB5, 0xBB, 0x0F, 0x2C, 0x3E, 0x0F, 0x36, 0x40, 0x53}));
  EXPECT_EQ(fc::primitives::encodeToString(addr3),
            std::string{"f2gfvuyh7v2sx3patm5k23wdzmhyhtmqctasbr23y"});

  EXPECT_OUTCOME_TRUE_2(
      addr4,
      fc::primitives::decode(std::vector<uint8_t>{
          0x0,  0x3,  0xAD, 0x58, 0xDF, 0x69, 0x6E, 0x2D, 0x4E, 0x91,
          0xEA, 0x86, 0xC8, 0x81, 0xE9, 0x38, 0xBA, 0x4E, 0xA8, 0x1B,
          0x39, 0x5E, 0x12, 0x79, 0x7B, 0x84, 0xB9, 0xCF, 0x31, 0x4B,
          0x95, 0x46, 0x70, 0x5E, 0x83, 0x9C, 0x7A, 0x99, 0xD6, 0x06,
          0xB2, 0x47, 0xDD, 0xB4, 0xF9, 0xAC, 0x7A, 0x34, 0x14, 0xDD}));
  EXPECT_EQ(fc::primitives::encodeToString(addr4),
            std::string{"f3vvmn62lofvhjd2ugzca6sof2j2ubwok6cj4xxbfzz4yuxfkgobpi"
                        "hhd2thlanmsh3w2ptld2gqkn2jvlss4a"});

  EXPECT_OUTCOME_TRUE_2(
      addr5,
      fc::primitives::decode(std::vector<uint8_t>{
          0x0,  0x3,  0xB3, 0x29, 0x4F, 0x0A, 0x2E, 0x29, 0xE0, 0xC6,
          0x6E, 0xBC, 0x23, 0x5D, 0x2F, 0xED, 0xCA, 0x56, 0x97, 0xBF,
          0x78, 0x4A, 0xF6, 0x05, 0xC7, 0x5A, 0xF6, 0x08, 0xE6, 0xA6,
          0x3D, 0x5C, 0xD3, 0x8E, 0xA8, 0x5C, 0xA8, 0x98, 0x9E, 0x0E,
          0xFD, 0xE9, 0x18, 0x8B, 0x38, 0x2F, 0x93, 0x72, 0x46, 0x0D}));
  EXPECT_EQ(fc::primitives::encodeToString(addr5),
            std::string{"f3wmuu6crofhqmm3v4enos73okk2l366ck6yc4owxwbdtkmpk42ohk"
                        "qxfitcpa57pjdcftql4tojda2poeruwa"});

  EXPECT_OUTCOME_TRUE_2(
      addr6, fc::primitives::decode(std::vector<uint8_t>{0x0, 0x0, 0x0, 0x0}));
  EXPECT_EQ(fc::primitives::encodeToString(addr6), std::string{"f00"});

  EXPECT_OUTCOME_TRUE_2(
      addr7,
      fc::primitives::decode(std::vector<uint8_t>{0x0, 0x0, 0x80, 0x08}));
  EXPECT_EQ(fc::primitives::encodeToString(addr7), std::string{"f01024"});

  EXPECT_OUTCOME_TRUE_2(addr8,
                        fc::primitives::decode(std::vector<uint8_t>{0x0,
                                                                    0x0,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0xFF,
                                                                    0x01}));
  EXPECT_EQ(fc::primitives::encodeToString(addr8),
            std::string{"f018446744073709551615"});
}

// Verify known addresses decoding from string
// https://filecoin-project.github.io/specs/#id-type-addresses
/**
 * @given A set of pairs (address_in_hex, address_base32_string)
 * @when Decoding addresses from strings
 * @then The outputs match expected strings
 */
TEST_F(AddressCodecTest, DecodeFromString) {
  EXPECT_OUTCOME_TRUE_2(addr,
                        fc::primitives::decodeFromString(std::string{
                            "f17uoq6tp427uzv7fztkbsnn64iwotfrristwpryy"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr),
            std::string{"f17uoq6tp427uzv7fztkbsnn64iwotfrristwpryy"});

  EXPECT_OUTCOME_TRUE_2(addr2,
                        fc::primitives::decodeFromString(std::string{
                            "f1xcbgdhkgkwht3hrrnui3jdopeejsoatkzmoltqy"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr2),
            std::string{"f1xcbgdhkgkwht3hrrnui3jdopeejsoatkzmoltqy"});

  EXPECT_OUTCOME_TRUE_2(addr3,
                        fc::primitives::decodeFromString(std::string{
                            "f2gfvuyh7v2sx3patm5k23wdzmhyhtmqctasbr23y"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr3),
            std::string{"f2gfvuyh7v2sx3patm5k23wdzmhyhtmqctasbr23y"});

  EXPECT_OUTCOME_TRUE_2(addr4,
                        fc::primitives::decodeFromString(std::string{
                            "f3vvmn62lofvhjd2ugzca6sof2j2ubwok6cj4xxbfzz4yuxfkg"
                            "obpihhd2thlanmsh3w2ptld2gqkn2jvlss4a"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr4),
            std::string{"f3vvmn62lofvhjd2ugzca6sof2j2ubwok6cj4xxbfzz4yuxfkgobpi"
                        "hhd2thlanmsh3w2ptld2gqkn2jvlss4a"});

  EXPECT_OUTCOME_TRUE_2(addr5,
                        fc::primitives::decodeFromString(std::string{
                            "f3wmuu6crofhqmm3v4enos73okk2l366ck6yc4owxwbdtkmpk4"
                            "2ohkqxfitcpa57pjdcftql4tojda2poeruwa"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr5),
            std::string{"f3wmuu6crofhqmm3v4enos73okk2l366ck6yc4owxwbdtkmpk42ohk"
                        "qxfitcpa57pjdcftql4tojda2poeruwa"});

  EXPECT_OUTCOME_TRUE_2(addr6,
                        fc::primitives::decodeFromString(std::string{"f00"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr6), std::string{"f00"});

  EXPECT_OUTCOME_TRUE_2(
      addr7, fc::primitives::decodeFromString(std::string{"f01024"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr7), std::string{"f01024"});

  EXPECT_OUTCOME_TRUE_2(
      addr8,
      fc::primitives::decodeFromString(std::string{"f018446744073709551615"}));
  EXPECT_EQ(fc::primitives::encodeToString(addr8),
            std::string{"f018446744073709551615"});
}
