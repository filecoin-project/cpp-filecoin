/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_encode_stream.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using fc::codec::cbor::CborEncodeStream;


template<typename T>
auto encodeOne(const T &&value) {
  return (CborEncodeStream() << value).single();
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

TEST(CborEncoder, List) {
  CborEncodeStream s;
  EXPECT_EQ(s.list(), "80"_unhex);
  s << 1;
  EXPECT_EQ(s.list(), "8101"_unhex);
}
