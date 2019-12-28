/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <unordered_map>

#include "codec/cbor/cbor.hpp"
#include "primitives/address/address_codec.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::primitives::address::Address;
using fc::primitives::address::AddressError;
using fc::primitives::address::decode;
using fc::primitives::address::decodeFromString;
using fc::primitives::address::encode;
using fc::primitives::address::encodeToString;
using fc::primitives::address::Network;
using fc::primitives::address::Protocol;

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
      {"t032104785", "00d1c2a70f"_unhex},
      {"t018446744073709551615", "00ffffffffffffffffff01"_unhex}};
};

// Address decoding from bytes
/**
 * @given A uint64_t number serialized as LEB128 byte sequence
 * @when Creating an ID Address for the given payload
 * @then ID Address successfully created
 */
TEST_F(AddressCodecTest, AddressDecodeOk) {
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
 * @given Array of 47 bytes reprsenting a BLS hash
 * @when Creating a BLS Address
 * @then INVALID_PAYLOAD error while creating the Address
 */
TEST_F(AddressCodecTest, InvalidPayloadSize) {
  int expected = static_cast<int>(AddressError::INVALID_PAYLOAD);
  EXPECT_OUTCOME_FALSE_2(
      error_code,
      decode(
          "03ceb343dd9694fcfe0f07b3b7f870fec1a7ea6abd7517fc65d33ce3a787a8aea869d99e36da3582c408e15e37421dc8"_unhex));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given An unsupported value as Address protocol
 * @when Creating an Address
 * @then Address is not created due to UNKNOWN_PROTOCOL error
 */
TEST_F(AddressCodecTest, UnknownProtocol) {
  int expected = static_cast<int>(AddressError::UNKNOWN_PROTOCOL);
  EXPECT_OUTCOME_FALSE_2(
      error_code, decode("042c39095318f8f2fd4b5927e2042bbd47af0fb4a0"_unhex));
  ASSERT_EQ(error_code.value(), expected);
}

/**
 * @given A non-ID address
 * @when Check the size of its checksum
 * @then The size is exactly 4 bytes
 */
TEST_F(AddressCodecTest, ChecksumSize) {
  EXPECT_OUTCOME_TRUE_2(
      addr, decode("01b0b5bf8e99bd815b35a29800d5a44e2d180c32b3"_unhex));
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
 * @given A set of addresses encoded as strings
 * @when Decoding addresses from string representation and re-encoding back to
 * string
 * @then The outputs match the original strings
 */
TEST_F(AddressCodecTest, RoundTripDecodeEncode) {
  for (auto it = this->knownAddresses.begin(); it != this->knownAddresses.end();
       it++) {
    EXPECT_OUTCOME_TRUE_2(addr, decodeFromString(it->first));
    EXPECT_EQ(encodeToString(addr), it->first);
  }
}

// CBOR encoding/decoding to stream
/**
 * @given An ID address and a Secp256k1 hash address
 * @when Encoding to CBOR and comparing with the known encodings
 * @then The outputs match expectations
 */
TEST_F(AddressCodecTest, MarshalCbor) {
  EXPECT_OUTCOME_TRUE_2(addr1, decode("0001"_unhex));
  CborEncodeStream s1;
  s1 << addr1;
  EXPECT_EQ(s1.data(), "420001"_unhex);

  EXPECT_OUTCOME_TRUE_2(
      addr2, decode("01fd1d0f4dfcd7e99afcb99a8326b7dc459d32c628"_unhex));
  CborEncodeStream s2;
  s2 << addr2;
  EXPECT_EQ(s2.data(), "5501fd1d0f4dfcd7e99afcb99a8326b7dc459d32c628"_unhex);
}

/**
 * @given A set of address-as-strings
 * @when Encoding to CBOR and decoding back, followed by re-encoding to string
 * @then The outputs match original strings
 */
TEST_F(AddressCodecTest, CborRoundTrip) {
  for (auto it = this->knownAddresses.begin(); it != this->knownAddresses.end();
       it++) {
    EXPECT_OUTCOME_TRUE_2(addr, decodeFromString(it->first));
    CborEncodeStream s;
    s << addr;
    Address addr2{};
    CborDecodeStream(s.data()) >> addr2;
    EXPECT_EQ(encodeToString(addr2), it->first);
  }
}
