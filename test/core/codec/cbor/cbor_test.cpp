/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_decode_stream.hpp"
#include "codec/cbor/cbor_encode_stream.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::codec::cbor::CborDecodeStream;
using fc::codec::cbor::CborEncodeStream;
using fc::codec::cbor::CborStreamType;

constexpr auto LIST = CborStreamType::LIST;
constexpr auto FLAT = CborStreamType::FLAT;

auto kCidRaw = "122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex;
auto kCidCbor = "D82A582300122031C3D57080D8463A3C63B2923DF5A1D40AD7A73EAE5A14AF584213E5F504AC33"_unhex;

template<typename T>
auto encodeOne(const T &value) {
  return (CborEncodeStream(FLAT) << value).data();
}

template<typename T>
void expectDecodeOne(const std::vector<uint8_t> &encoded, const T &expected) {
  T actual;
  CborDecodeStream(encoded) >> actual;
  EXPECT_EQ(actual, expected);
}

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
  CborEncodeStream s{FLAT};
  EXPECT_EQ(s.data(), ""_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "01"_unhex);
  s << 2;
  EXPECT_EQ(s.data(), "0102"_unhex);
}

TEST(CborEncoder, List) {
  CborEncodeStream s{LIST};
  EXPECT_EQ(s.data(), "80"_unhex);
  s << 1;
  EXPECT_EQ(s.data(), "8101"_unhex);
}

TEST(CborEncoder, ListNest) {
  CborEncodeStream s{LIST};
  EXPECT_EQ(s.data(), "80"_unhex);
  s << (CborEncodeStream(LIST) << 1 << 2);
  EXPECT_EQ(s.data(), "81820102"_unhex);
  s << (CborEncodeStream(FLAT) << 3 << 4 << 5);
  EXPECT_EQ(s.data(), "84820102030405"_unhex);
}

TEST(CborEncoder, FlatNest) {
  CborEncodeStream s1{FLAT};
  s1 << 1 << 2;
  EXPECT_EQ(s1.data(), "0102"_unhex);
  CborEncodeStream s2{FLAT};
  s2 << s1;
  EXPECT_EQ(s2.data(), "0102"_unhex);
}

TEST(CborEncoder, Cid) {
  EXPECT_OUTCOME_TRUE_2(cid, libp2p::multi::ContentIdentifierCodec::decode(kCidRaw));
  EXPECT_EQ(encodeOne(cid), kCidCbor);
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
