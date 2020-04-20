/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/libp2p/cbor_buffering.hpp"

#include <gtest/gtest.h>

#include "testutil/cbor.hpp"

struct CborBufferingTest : testing::Test {
  void SetUp() override {
    auto s = fc::codec::cbor::CborEncodeStream::list();
    s << -1 << 1 << 0x80 << 0x8000 << 0x800000 << 0x80000000;
    s << "123"
      << "dead"_unhex;
    auto m = s.map();
    m["key"] << (s.list() << "010001020001"_cid);
    s << m;
    buffer = s.data();
  }

  std::vector<uint8_t> buffer;
  fc::common::libp2p::CborBuffering buffering;
};

TEST_F(CborBufferingTest, ConsumeAll) {
  EXPECT_TRUE(buffering.done());
  EXPECT_EQ(buffering.moreBytes(), 0);
  buffering.reset();
  EXPECT_FALSE(buffering.done());
  EXPECT_NE(buffering.moreBytes(), 0);
  EXPECT_OUTCOME_EQ(buffering.consume(buffer), buffer.size());
  EXPECT_TRUE(buffering.done());
  EXPECT_EQ(buffering.moreBytes(), 0);
}

TEST_F(CborBufferingTest, ConsumeEachByte) {
  buffering.reset();
  for (size_t i = 0; i < buffer.size(); ++i) {
    EXPECT_FALSE(buffering.done());
    EXPECT_NE(buffering.moreBytes(), 0);
    EXPECT_OUTCOME_EQ(buffering.consume(gsl::make_span(buffer).subspan(i, 1)),
                      1);
  }
  EXPECT_TRUE(buffering.done());
  EXPECT_EQ(buffering.moreBytes(), 0);
}
