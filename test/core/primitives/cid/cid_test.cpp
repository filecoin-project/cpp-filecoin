/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/cid.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::primitives::cid {

  /**
   * @given cid v0 and prefix from go-cid
   * @when get prefix of the cid
   * @then expected cid prefix returned
   */
  TEST(CidTest, PrefixV0) {
    CID cid{
        "12202d5bb7c3afbe68c05bcd109d890dca28ceb0105bf529ea1111f9ef8b44b217b9"_cid};
    EXPECT_EQ(cid.version, CID::Version::V0);
    EXPECT_OUTCOME_EQ(cid.getPrefix(), "00701220"_unhex);
  }

  /**
   * @given cid v1 and prefix from go-cid
   * @when get prefix of the cid
   * @then expected cid prefix returned
   */
  TEST(CidTest, PrefixV1) {
    CID cid{
        "017112202d5bb7c3afbe68c05bcd109d890dca28ceb0105bf529ea1111f9ef8b44b217b9"_cid};
    EXPECT_EQ(cid.version, CID::Version::V1);
    EXPECT_OUTCOME_EQ(cid.getPrefix(), "01711220"_unhex);
  }

}  // namespace fc::primitives::cid
