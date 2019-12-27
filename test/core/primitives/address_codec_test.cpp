/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <algorithm>

#include "primitives/address/address_codec.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::primitives::address::Address;
using fc::primitives::address::AddressError;
using fc::primitives::address::decode;
using fc::primitives::address::decodeFromString;
using fc::primitives::address::encode;
using fc::primitives::address::encodeToString;
using fc::primitives::address::Network;
using fc::primitives::address::Protocol;

std::vector<uint8_t> randomNBytes(uint N) {
  std::vector<int> v(N);
  std::generate(v.begin(), v.end(), std::rand);
  std::vector<uint8_t> u(N);
  std::transform(
      v.begin(), v.end(), u.begin(), [](int k) -> uint8_t { return k % 256; });
  return u;
}

struct AddressCodecTest : public testing::Test {
  std::unordered_map<std::string, std::vector<uint8_t>> knownAddresses{
      {"t17uoq6tp427uzv7fztkbsnn64iwotfrristwpryy",
       "01fd1d0f4dfcd7e99afcb99a8326b7dc459d32c628"_unhex},
      {"t1xcbgdhkgkwht3hrrnui3jdopeejsoatkzmoltqy",
       "01b882619d46558f3d9e316d11b48dcf211327026a"_unhex},
      {"t2gfvuyh7v2sx3patm5k23wdzmhyhtmqctasbr23y",
       "02316b4c1ff5d4afb7826ceab5bb0f2c3e0f364053"_unhex},
      {"t3vvmn62lofvhjd2ugzca6sof2j2ubwok6cj4xxbfzz4yuxfkgobpihhd2thlanmsh3w2pt"
       "ld2gqkn2jvlss4a",
       "03ad58df696e2d4e91ea86c881e938ba4ea81b395e12797b84b9cf314b9546705e839c7a99d606b247ddb4f9ac7a3414dd"_unhex},
      {"t3wmuu6crofhqmm3v4enos73okk2l366ck6yc4owxwbdtkmpk42ohkqxfitcpa57pjdcftq"
       "l4tojda2poeruwa",
       "03b3294f0a2e29e0c66ebc235d2fedca5697bf784af605c75af608e6a63d5cd38ea85ca8989e0efde9188b382f9372460d"_unhex},
      {"t00", "0000"_unhex},
      {"t01024", "008008"_unhex},
      {"t018446744073709551615", "00ffffffffffffffffff01"_unhex}};
};

// Address decoding from bytes
/**
 * @given A uint64_t number serialized as LEB128 byte sequence
 * @when Creating an ID Address for the given payload
 * @then ID Address successfully created
 */
TEST_F(AddressCodecTest, AddressDecodeOK) {
  EXPECT_OUTCOME_TRUE_2(
      addr,
      decode("00d1c2a70f"_unhex));  // {protocol == ID, payload == 32104785}
  EXPECT_EQ(boost::get<uint64_t>(addr.data), 32104785);
}

/**
 * @given A LEB128 encoding of some big number (greater than uint64_t can fit)
 * @when Creating an ID Address for the given payload
 * @then INVALID_PAYLOAD error occurs while decoding the payload into a uint64
 * integer
 */
TEST_F(AddressCodecTest, AddressDecodeFailure) {
  int expected = static_cast<int>(AddressError::INVALID_PAYLOAD);
  EXPECT_OUTCOME_FALSE_2(error_code,
                         decode("008080808080808080808001"_unhex));  // 2^70
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given Array of 20 bytes reprsenting a blake2b-160 hash
 * @when Creating a Secp256k1 Address
 * @then Secp256k1 Address successfully created
 */
TEST_F(AddressCodecTest, AddressDecodeSecp256k1OK) {
  std::vector<uint8_t> payload = randomNBytes(20);
  std::vector<uint8_t> bytes{0x1};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_TRUE_1(decode(bytes));
}

/**
 * @given Array of 47 bytes reprsenting a BLS hash
 * @when Creating a BLS Address
 * @then INVALID_PAYLOAD error while creating the Address
 */
TEST_F(AddressCodecTest, InvalidPayloadSize) {
  std::vector<uint8_t> payload = randomNBytes(47);
  std::vector<uint8_t> bytes{0x3};
  int expected = static_cast<int>(AddressError::INVALID_PAYLOAD);
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_FALSE_2(error_code, decode(bytes));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given An unsupported value as Address protocol
 * @when Creating an Address
 * @then Address is not created due to UNKNOWN_PROTOCOL error
 */
TEST_F(AddressCodecTest, UnknownProtocol) {
  std::vector<uint8_t> payload = randomNBytes(20);
  std::vector<uint8_t> bytes{0x4};
  int expected = static_cast<int>(AddressError::UNKNOWN_PROTOCOL);
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_FALSE_2(error_code, decode(bytes));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given An ID Address
 * @when Encoding it to std::string
 * @then The output matches expected form "t0XXXXXXXX"
 */
TEST_F(AddressCodecTest, IDtoString) {
  EXPECT_OUTCOME_TRUE_2(addr, decode("00d1c2a70f"_unhex))  // 32104785
  EXPECT_EQ(encodeToString(addr), std::string{"t032104785"});
}

/**
 * @given A non-ID address
 * @when Check the size of its checksum
 * @then The size is exactly 4 bytes
 */
TEST_F(AddressCodecTest, ChecksumSize) {
  std::vector<uint8_t> payload = randomNBytes(20);
  std::vector<uint8_t> bytes{0x1};
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  EXPECT_OUTCOME_TRUE_2(addr, decode(bytes));
  ASSERT_TRUE(checksum(addr).size() == 4);
}

// Verify known addresses serializations to string match expectation
// https://filecoin-project.github.io/specs/#id-type-addresses
/**
 * @given A set of pairs (address_in_hex, address_base32_string)
 * @when Serializing addresses to strings
 * @then The outputs match expected strings
 */
TEST_F(AddressCodecTest, EncodeToString) {
  for (auto it = this->knownAddresses.begin(); it != this->knownAddresses.end();
       it++) {
    EXPECT_OUTCOME_TRUE_2(addr, decode(it->second));
    EXPECT_EQ(encodeToString(addr), it->first);
  }
}

// Verify known addresses decoding from string
// https://filecoin-project.github.io/specs/#id-type-addresses
/**
 * @given A set of pairs (address_in_hex, address_base32_string)
 * @when Decoding addresses from strings
 * @then The outputs match expected strings
 */
TEST_F(AddressCodecTest, RoundTripDecodeEncode) {
  for (auto it = this->knownAddresses.begin(); it != this->knownAddresses.end();
       it++) {
    EXPECT_OUTCOME_TRUE_2(addr, decodeFromString(it->first));
    EXPECT_EQ(encodeToString(addr), it->first);
  }
}
