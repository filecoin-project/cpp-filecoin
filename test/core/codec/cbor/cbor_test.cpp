/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"
#include "codec/cbor/cbor_resolve.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::codec::cbor::CborDecodeError;
using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::codec::cbor::CborResolveError;
using fc::codec::cbor::resolve;
using libp2p::multi::ContentIdentifier;
using libp2p::multi::ContentIdentifierCodec;

auto kCidEmptyRaw = "122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC00"_unhex;
auto kCidRaw = "122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex;
auto kCidCbor = "D82A582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex;

template<typename T>
auto encodeOne(const T &value) {
  return (CborEncodeStream() << value).data();
}

template<typename T>
void expectDecodeOne(const std::vector<uint8_t> &encoded, const T &expected) {
  T actual;
  CborDecodeStream(encoded) >> actual;
  EXPECT_EQ(actual, expected);
}

#define EXPECT_OUTCOME_RAISE(ecode, statement) \
  try { statement; FAIL() << "Line " << __LINE__ << ": " << #ecode << " not raised"; } \
  catch (std::system_error &e) { EXPECT_EQ(e.code(), ecode); }

#define EXPECT_OUTCOME_ERROR(ecode, expr) \
  { EXPECT_OUTCOME_FALSE_2(e, expr); EXPECT_EQ(e, ecode); }

TEST(CborEncoder, Integral) {
  EXPECT_EQ(encodeOne(0ull), "00"_unhex);
  EXPECT_EQ(encodeOne(0ll), "00"_unhex);
  EXPECT_EQ(encodeOne(1), "01"_unhex);
  EXPECT_EQ(encodeOne(23), "17"_unhex);
  EXPECT_EQ(encodeOne(24), "1818"_unhex);
  EXPECT_EQ(encodeOne(-1), "20"_unhex);
  EXPECT_EQ(encodeOne(false), "F4"_unhex);
  EXPECT_EQ(encodeOne(true), "F5"_unhex);
}

TEST(CborEncoder, Flat) {
  CborEncodeStream s;
  EXPECT_EQ(s.data(), ""_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "01"_unhex);
  s << 2;
  EXPECT_EQ(s.data(), "0102"_unhex);
}

TEST(CborEncoder, List) {
  auto s = CborEncodeStream().list();
  EXPECT_EQ(s.data(), "80"_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "8101"_unhex);
}

TEST(CborEncoder, ListNest) {
  auto s = CborEncodeStream().list();
  EXPECT_EQ(s.data(), "80"_unhex);
  s << (s.list() << 1 << 2);
  EXPECT_EQ(s.data(), "81820102"_unhex);
  s << (CborEncodeStream() << 3 << 4 << 5);
  EXPECT_EQ(s.data(), "84820102030405"_unhex);
}

TEST(CborEncoder, FlatNest) {
  CborEncodeStream s1;
  s1 << 1 << 2;
  EXPECT_EQ(s1.data(), "0102"_unhex);
  CborEncodeStream s2;
  s2 << s1;
  EXPECT_EQ(s2.data(), "0102"_unhex);
}

TEST(CborEncoder, Cid) {
  EXPECT_OUTCOME_TRUE_2(cid, ContentIdentifierCodec::decode(kCidRaw));
  EXPECT_EQ(encodeOne(cid), kCidCbor);
}

TEST(CborEncoder, String) {
  EXPECT_EQ(encodeOne(std::string("foo")), "63666F6F"_unhex);
}

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

TEST(CborDecoder, Cid) {
  EXPECT_OUTCOME_TRUE_2(expected, ContentIdentifierCodec::decode(kCidRaw));
  // libp2p::multi::ContentIdentifier is not default constructible and must be initialized somehow
  EXPECT_OUTCOME_TRUE_2(actual, ContentIdentifierCodec::decode(kCidEmptyRaw));
  EXPECT_NE(actual, expected);
  CborDecodeStream(kCidCbor) >> actual;
  EXPECT_EQ(actual, expected);
}

TEST(CborDecoder, CidNext) {
  auto bytes = kCidCbor;
  bytes.push_back(0x01);
  CborDecodeStream s(bytes);
  s.next();
  int i;
  s >> i;
}

TEST(CborDecoder, Flat) {
  CborDecodeStream s("0504"_unhex);
  int a, b;
  s >> a >> b;
  EXPECT_EQ(a, 5);
  EXPECT_EQ(b, 4);
}

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

TEST(CborDecoder, String) {
  std::string s;
  CborDecodeStream("63666F6F"_unhex) >> s;
  EXPECT_EQ(s, "foo");
}

TEST(CborDecoder, Map) {
  auto m = CborDecodeStream("A261610261628101"_unhex).map();
  int a, b;
  m.at("a") >> a;
  m.at("b").list() >> b;
}

TEST(CborDecoder, InitErrors) {
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR, CborDecodeStream("FF"_unhex));
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR, CborDecodeStream("18"_unhex));
}

TEST(CborDecoder, IntErrors) {
  bool b;
  uint8_t u8;
  int8_t i8;
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE, CborDecodeStream("01"_unhex) >> b);
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE, CborDecodeStream("80"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW, CborDecodeStream("21"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW, CborDecodeStream("190100"_unhex) >> u8);
  EXPECT_OUTCOME_RAISE(CborDecodeError::INT_OVERFLOW, CborDecodeStream("1880"_unhex) >> i8);
}

TEST(CborDecoder, FlatErrors) {
  int i;
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE, CborDecodeStream("01"_unhex).list() >> i >> i);
}

TEST(CborDecoder, ListErrors) {
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE, CborDecodeStream("01"_unhex).list());
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR, CborDecodeStream("81"_unhex).list());
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR, CborDecodeStream("8018"_unhex).list());
  int i;
  EXPECT_OUTCOME_RAISE(CborDecodeError::WRONG_TYPE, CborDecodeStream("80"_unhex).list() >> i);
}

TEST(CborDecoder, CidErrors) {
  // libp2p::multi::ContentIdentifier is not default constructible and must be initialized somehow
  EXPECT_OUTCOME_TRUE_2(actual, ContentIdentifierCodec::decode(kCidEmptyRaw));
  // no tag
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR_CID, CborDecodeStream("582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex) >> actual);
  // not 42 tag
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR_CID, CborDecodeStream("D82B582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex) >> actual);
  // empty 42 tag
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR, CborDecodeStream("D82A"_unhex) >> actual);
  // not bytes
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR_CID, CborDecodeStream("D82B01"_unhex) >> actual);
  // no multibase 00 prefix
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CBOR_CID, CborDecodeStream("D82A5822122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex) >> actual);
  // invalid cid
  EXPECT_OUTCOME_RAISE(CborDecodeError::INVALID_CID, CborDecodeStream("D82A420000"_unhex) >> actual);
}

TEST(CborDecoder, IsCid) {
  EXPECT_TRUE(CborDecodeStream(kCidCbor).isCid());
  EXPECT_TRUE(CborDecodeStream("D82A"_unhex).isCid());
  EXPECT_FALSE(CborDecodeStream("01"_unhex).isCid());
}

TEST(CborDecoder, Misc) {
  EXPECT_TRUE(CborDecodeStream("80"_unhex).isList());
  EXPECT_EQ(CborDecodeStream("820101"_unhex).listLength(), 2);
  EXPECT_FALSE(CborDecodeStream("80"_unhex).isMap());
  EXPECT_TRUE(CborDecodeStream("A0"_unhex).isMap());
  EXPECT_EQ(CborDecodeStream("810201"_unhex).raw(), "8102"_unhex);
}

TEST(CborResolve, Root) {
  auto a = "80"_unhex;
  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {}));
  EXPECT_EQ(b.first, a);
  EXPECT_EQ(b.second.size(), 0);
}

TEST(CborResolve, Cid) {
  EXPECT_OUTCOME_TRUE_2(b, resolve(kCidCbor, {"a"}));
  EXPECT_EQ(b.first, kCidCbor);
  EXPECT_EQ(b.second.size(), 1);
}

TEST(CborResolve, IntKey) {
  auto a = "8405060708"_unhex;

  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {"2"}));
  EXPECT_EQ(b.first, "07"_unhex);
  EXPECT_EQ(b.second.size(), 0);

  EXPECT_OUTCOME_ERROR(CborResolveError::INT_KEY_EXPECTED, resolve(a, {"a"}));
  EXPECT_OUTCOME_ERROR(CborResolveError::INT_KEY_EXPECTED, resolve(a, {"1a"}));
  EXPECT_OUTCOME_ERROR(CborResolveError::KEY_NOT_FOUND, resolve(a, {"4"}));
}

TEST(CborResolve, StringKey) {
  auto a = "A3616103616204616305"_unhex;

  EXPECT_OUTCOME_TRUE_2(b, resolve(a, {"b"}));
  EXPECT_EQ(b.first, "04"_unhex);
  EXPECT_EQ(b.second.size(), 0);

  EXPECT_OUTCOME_ERROR(CborResolveError::KEY_NOT_FOUND, resolve(a, {"1"}));
}

TEST(CborResolve, Errors) {
  EXPECT_OUTCOME_ERROR(CborResolveError::CONTAINER_EXPECTED, resolve("01"_unhex, {"0"}));
  EXPECT_OUTCOME_ERROR(CborDecodeError::INVALID_CBOR, resolve("8281"_unhex, {"1"}));
}
