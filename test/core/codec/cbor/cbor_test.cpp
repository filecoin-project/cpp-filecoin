/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_encode_stream.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using fc::codec::cbor::CborEncodeStream;
using fc::codec::cbor::CborStreamType;

constexpr auto LIST = CborStreamType::LIST;
constexpr auto SINGLE = CborStreamType::SINGLE;
constexpr auto FLAT = CborStreamType::FLAT;

template<typename T>
auto encodeOne(const T &&value) {
  return (CborEncodeStream(SINGLE) << value).data();
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
