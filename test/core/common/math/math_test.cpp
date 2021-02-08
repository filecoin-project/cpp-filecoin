/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/math/math.hpp"

#include <gtest/gtest.h>
#include <fstream>
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"

namespace fc::common::math {

  /**
   * Test expneg with Q.128 format against
   * 'specs-actors/actor/builtin/reward/testdata/TestExpFunction.golden'
   */
  TEST(Math, ExpNegTest128) {
    const auto pairs =
        parseCsvPair(resourcePath("common/math/test_exp_function_q128.txt"));

    for (const auto &p : pairs) {
      ASSERT_EQ(p.second, expneg(p.first));
    }
  }

  /**
   * Test expneg with Q.256 format against
   * 'lotus/chain/types/testdata/TestExpFunction.golden'
   */
  TEST(Math, ExpNegTest256) {
    const auto pairs =
        parseCsvPair(resourcePath("common/math/test_exp_function_q256.txt"));
    const uint precision = 256;

    for (const auto &p : pairs) {
      ASSERT_EQ(p.second, expneg(p.first, precision));
    }
  }

}  // namespace fc::common::math
