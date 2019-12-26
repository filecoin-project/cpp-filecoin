/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor.hpp"
#include "primitives/big_int.hpp"

#include "common/invalid_cid.hpp"
#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::codec::cbor::CborDecodeError;
using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeError;
using fc::codec::cbor::CborEncodeStream;
using fc::codec::cbor::CborResolveError;
using fc::codec::cbor::decode;
using fc::codec::cbor::encode;
using fc::codec::cbor::resolve;
using fc::common::kInvalidCid;

auto kCidRaw =
    libp2p::multi::ContentIdentifierCodec::decode(
        "122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex)
        .value();
auto kCidCbor =
    "D82A582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex;

template <typename T>
void expectDecodeOne(const std::vector<uint8_t> &encoded, const T &expected) {
  EXPECT_OUTCOME_EQ(decode<T>(encoded), expected);
}

/**
 * @given Element or CBOR
 * @when encode decode
 * @then As expected
 */
TEST(Cbor, EncodeDecode) {
  EXPECT_OUTCOME_EQ(encode(1), "01"_unhex);
  EXPECT_OUTCOME_EQ(decode<int>("01"_unhex), 1);
  EXPECT_OUTCOME_EQ(encode(decode<int>("01"_unhex).value()), "01"_unhex);
  EXPECT_OUTCOME_EQ(decode<int>(encode(1).value()), 1);
  EXPECT_OUTCOME_ERROR(CborDecodeError::WRONG_TYPE, decode<int>("80"_unhex));
}

/** BigInt CBOR encoding and decoding */
TEST(Cbor, BigInt) {
  using fc::primitives::BigInt;
  EXPECT_OUTCOME_EQ(encode(BigInt(0xCAFE)), "4300CAFE"_unhex);
  EXPECT_OUTCOME_EQ(decode<BigInt>("4300CAFE"_unhex), 0xCAFE);
  EXPECT_OUTCOME_EQ(encode(BigInt(-0xCAFE)), "4301CAFE"_unhex);
  EXPECT_OUTCOME_EQ(decode<BigInt>("4301CAFE"_unhex), -0xCAFE);
  EXPECT_OUTCOME_EQ(encode(BigInt(0)), "40"_unhex);
  EXPECT_OUTCOME_EQ(decode<BigInt>("40"_unhex), 0);
}

/** UBigInt CBOR encoding and decoding */
TEST(Cbor, UBigInt) {
  using fc::primitives::UBigInt;
  EXPECT_OUTCOME_EQ(encode(UBigInt(0xCAFE)), "42CAFE"_unhex);
  EXPECT_OUTCOME_EQ(decode<UBigInt>("42CAFE"_unhex), 0xCAFE);
  EXPECT_OUTCOME_EQ(encode(UBigInt(0)), "40"_unhex);
  EXPECT_OUTCOME_EQ(decode<UBigInt>("40"_unhex), 0);
}

/** Null CBOR encoding and decoding */
TEST(Cbor, Null) {
  EXPECT_OUTCOME_EQ(encode(nullptr), "F6"_unhex);
  EXPECT_TRUE(CborDecodeStream("F6"_unhex).isNull());
  EXPECT_FALSE(CborDecodeStream("01"_unhex).isNull());
}

/**
 * @given Integers and bool
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, Integral) {
  EXPECT_OUTCOME_EQ(encode(0ull), "00"_unhex);
  EXPECT_OUTCOME_EQ(encode(0ll), "00"_unhex);
  EXPECT_OUTCOME_EQ(encode(1), "01"_unhex);
  EXPECT_OUTCOME_EQ(encode(23), "17"_unhex);
  EXPECT_OUTCOME_EQ(encode(24), "1818"_unhex);
  EXPECT_OUTCOME_EQ(encode(-1), "20"_unhex);
  EXPECT_OUTCOME_EQ(encode(false), "F4"_unhex);
  EXPECT_OUTCOME_EQ(encode(true), "F5"_unhex);
}

/**
 * @given Sequence
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, Flat) {
  CborEncodeStream s;
  EXPECT_EQ(s.data(), ""_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "01"_unhex);
  s << 2;
  EXPECT_EQ(s.data(), "0102"_unhex);
}

/**
 * @given List
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, List) {
  auto s = CborEncodeStream().list();
  EXPECT_EQ(s.data(), "80"_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "8101"_unhex);
}

/**
 * @given Nested list and sequence containers
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, ListNest) {
  auto s = CborEncodeStream().list();
  EXPECT_EQ(s.data(), "80"_unhex);
  s << (s.list() << 1 << 2);
  EXPECT_EQ(s.data(), "81820102"_unhex);
  s << (CborEncodeStream() << 3 << 4 << 5);
  EXPECT_EQ(s.data(), "84820102030405"_unhex);
}

/**
 * @given Nested sequence containers
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, FlatNest) {
  CborEncodeStream s1;
  s1 << 1 << 2;
  EXPECT_EQ(s1.data(), "0102"_unhex);
  CborEncodeStream s2;
  s2 << s1;
  EXPECT_EQ(s2.data(), "0102"_unhex);
}

/**
 * @given CID
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, Cid) {
  EXPECT_OUTCOME_EQ(encode(kCidRaw), kCidCbor);
}

/**
 * @given String
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, String) {
  EXPECT_OUTCOME_EQ(encode(std::string("foo")), "63666F6F"_unhex);
}

/**
 * @given Bytes
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, Bytes) {
  EXPECT_OUTCOME_EQ(encode("CAFE"_unhex), "42CAFE"_unhex);
}

/**
 * @given Map container
 * @when Encode
 * @then Encoded as expected
 */
TEST(CborEncoder, Map) {
  CborEncodeStream s;
  auto map = s.map();
  map["aa"] << 1;
  map["b"] << 2;
  map["c"] << 3;
  s << map;
  EXPECT_EQ(s.data(), "A361620261630362616101"_unhex);
}

/**
 * @given Invalid CID
 * @when Encode
 * @then Error
 */
TEST(CborEncoder, CidErrors) {
  EXPECT_OUTCOME_ERROR(CborEncodeError::INVALID_CID, encode(kInvalidCid));
}

/**
 * @given Invalid map container
 * @when Encode
 * @then Error
 */
TEST(CborEncoder, MapErrors) {
  CborEncodeStream s;
  auto map1 = s.map();
  map1["a"] << 1 << 2;
  EXPECT_OUTCOME_RAISE(CborEncodeError::EXPECTED_MAP_VALUE_SINGLE,
                       CborEncodeStream() << map1);
  auto map2 = s.map();
  map2["a"];
  EXPECT_OUTCOME_RAISE(CborEncodeError::EXPECTED_MAP_VALUE_SINGLE,
                       CborEncodeStream() << map2);
}

/**
 * @given Integer and bool CBOR
 * @when Decode integer and bool
 * @then Decoded as expected
 */
TEST(CborDecoder, Integral) {
  expectDecodeOne("00"_unhex, 0ull);
  expectDecodeOne("00"_unhex, 0ll);
  expectDecodeOne("01"_unhex, 1);
  expectDecodeOne("17"_unhex, 23);
  expectDecodeOne("1818"_unhex, 24);
  expectDecodeOne("20"_unhex, -1);
  expectDecodeOne("F4"_unhex, false);
  expectDecodeOne("F5"_unhex, true);
}

/**
 * @given CID CBOR
 * @when Decode CID
 * @then Decoded as expected
 */
TEST(CborDecoder, Cid) {
  auto actual = kInvalidCid;
  EXPECT_NE(actual, kCidRaw);
  CborDecodeStream(kCidCbor) >> actual;
  EXPECT_EQ(actual, kCidRaw);
}

/**
 * @given CID CBOR
 * @when Skip CID
 * @then Skipped as expected
 */
TEST(CborDecoder, CidNext) {
  auto bytes = kCidCbor;
  bytes.push_back(0x01);
  CborDecodeStream s(bytes);
  s.next();
  int i;
  s >> i;
}

/**
 * @given Sequence CBOR
 * @when Decode sequence
 * @then Decoded as expected
 */
TEST(CborDecoder, Flat) {
  CborDecodeStream s("0504"_unhex);
  int a, b;
  s >> a >> b;
  EXPECT_EQ(a, 5);
  EXPECT_EQ(b, 4);
}

/**
 * @given List CBOR
 * @when Decode list container
 * @then Decoded as expected
 */
TEST(CborDecoder, List) {
  CborDecodeStream s1("82050403"_unhex);
  auto s2 = s1.list();
  int a, b;
  s2 >> a >> b;
  EXPECT_EQ(a, 5);
  EXPECT_EQ(b, 4);
  int c;
  s1 >> c;
  EXPECT_EQ(c, 3);
}

/**
 * @given String CBOR
 * @when Decode string
 * @then Decoded as expected
 */
TEST(CborDecoder, String) {
  std::string s;
  CborDecodeStream("63666F6F"_unhex) >> s;
  EXPECT_EQ(s, "foo");
}

/**
 * @given Map CBOR
 * @when Decode map container
 * @then Decoded as expected
 */
TEST(CborDecoder, Map) {
  auto m = CborDecodeStream("A261610261628101"_unhex).map();
  int a, b;
  m.at("a") >> a;
  m.at("b").list() >> b;
}

/**
 * @given Invalid CBOR
 * @when Init decoder
 * @then Error
 */
TEST(CborDecoder, InitErrors) {
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR,
                       CborDecodeStream("FF"_unhex));
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR,
                       CborDecodeStream("18"_unhex));
}

/**
 * @given Invalid CBOR or wrong type
 * @when Decode integer and bool
 * @then Error
 */
TEST(CborDecoder, IntErrors) {
  bool b;
  uint8_t u8;
  int8_t i8;
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE,
                       CborDecodeStream("01"_unhex) >> b);
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE,
                       CborDecodeStream("80"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW,
                       CborDecodeStream("21"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW,
                       CborDecodeStream("190100"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW,
                       CborDecodeStream("1880"_unhex) >> i8);
}

/**
 * @given Sequence and list CBOR
 * @when Decode after end of sequence or list
 * @then Error
 */
TEST(CborDecoder, FlatErrors) {
  int i;
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE,
                       CborDecodeStream("01"_unhex).list() >> i >> i);
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE,
                       CborDecodeStream("80"_unhex).list() >> i);
}

/**
 * @given Invalid list CBOR
 * @when Decode list container
 * @then Error
 */
TEST(CborDecoder, ListErrors) {
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE,
                       CborDecodeStream("01"_unhex).list());
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR,
                       CborDecodeStream("81"_unhex).list());
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR,
                       CborDecodeStream("8018"_unhex).list());
}

/**
 * @given Invalid CID CBOR
 * @when Decode CID
 * @then Error
 */
TEST(CborDecoder, CidErrors) {
  auto actual = kInvalidCid;
  // no tag
  EXPECT_OUTCOME_RAISE(
      CborDecodeError::INVALID_CBOR_CID,
      CborDecodeStream(
          "582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex)
          >> actual);
  // not 42 tag
  EXPECT_OUTCOME_RAISE(
      CborDecodeError::INVALID_CBOR_CID,
      CborDecodeStream(
          "D82B582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex)
          >> actual);
  // empty 42 tag
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR,
                       CborDecodeStream("D82A"_unhex) >> actual);
  // not bytes
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR_CID,
                       CborDecodeStream("D82B01"_unhex) >> actual);
  // no multibase 00 prefix
  EXPECT_OUTCOME_RAISE(
      CborDecodeError::INVALID_CBOR_CID,
      CborDecodeStream(
          "D82A5822122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex)
          >> actual);
  // invalid cid
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CID,
                       CborDecodeStream("D82A420000"_unhex) >> actual);
}

/**
 * @given CBOR
 * @when isCid
 * @then As expected
 */
TEST(CborDecoder, IsCid) {
  EXPECT_TRUE(CborDecodeStream(kCidCbor).isCid());
  EXPECT_TRUE(CborDecodeStream("D82A"_unhex).isCid());
  EXPECT_FALSE(CborDecodeStream("01"_unhex).isCid());
}

/**
 * @given CBOR
 * @when isList, listLength, isMap, raw
 * @then As expected
 */
TEST(CborDecoder, Misc) {
  EXPECT_TRUE(CborDecodeStream("80"_unhex).isList());
  EXPECT_EQ(CborDecodeStream("820101"_unhex).listLength(), 2);
  EXPECT_FALSE(CborDecodeStream("80"_unhex).isMap());
  EXPECT_TRUE(CborDecodeStream("A0"_unhex).isMap());
  EXPECT_EQ(CborDecodeStream("810201"_unhex).raw(), "8102"_unhex);
}

/**
 * @given CBOR and empty path
 * @when Resolve
 * @then Returned whole CBOR
 */
TEST(CborResolve, Root) {
  auto a = "80"_unhex;
  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {}));
  EXPECT_EQ(b.first, a);
  EXPECT_EQ(b.second.size(), 0);
}

/**
 * @given CBOR and path through CID
 * @when Resolve
 * @then Returns CID CBOR and rest of path
 */
TEST(CborResolve, Cid) {
  EXPECT_OUTCOME_TRUE_2(b, resolve(kCidCbor, {"a"}));
  EXPECT_EQ(b.first, kCidCbor);
  EXPECT_EQ(b.second.size(), 1);
}

/**
 * @given List CBOR and path
 * @when Resolve
 * @then As expected
 */
TEST(CborResolve, IntKey) {
  auto a = "8405060708"_unhex;

  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {"2"}));
  EXPECT_EQ(b.first, "07"_unhex);
  EXPECT_EQ(b.second.size(), 0);

  EXPECT_OUTCOME_ERROR(CborResolveError::INT_KEY_EXPECTED, resolve(a, {"a"}));
  EXPECT_OUTCOME_ERROR(CborResolveError::INT_KEY_EXPECTED, resolve(a, {"1a"}));
  EXPECT_OUTCOME_ERROR(CborResolveError::KEY_NOT_FOUND, resolve(a, {"4"}));
}

/**
 * @given Map CBOR and path
 * @when Resolve
 * @then As expected
 */
TEST(CborResolve, StringKey) {
  auto a = "A3616103616204616305"_unhex;

  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {"b"}));
  EXPECT_EQ(b.first, "04"_unhex);
  EXPECT_EQ(b.second.size(), 0);

  EXPECT_OUTCOME_ERROR(CborResolveError::KEY_NOT_FOUND, resolve(a, {"1"}));
}

/**
 * @given Invalid CBOR or wrong type
 * @when Resolve
 * @then Error
 */
TEST(CborResolve, Errors) {
  EXPECT_OUTCOME_ERROR(CborResolveError::CONTAINER_EXPECTED,
                       resolve("01"_unhex, {"0"}));
  EXPECT_OUTCOME_ERROR(CborDecodeError::INVALID_CBOR,
                       resolve("8281"_unhex, {"1"}));
}
