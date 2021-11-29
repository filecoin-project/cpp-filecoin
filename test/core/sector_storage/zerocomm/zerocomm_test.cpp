/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sector_storage/zerocomm/zerocomm.hpp"
#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

namespace fc::sector_storage::zerocomm {

  struct Params {
    Params(uint64_t size, const std::string &cid_str)
        : unpadded_size{size}, expected{CID::fromString(cid_str).value()} {}

    UnpaddedPieceSize unpadded_size;
    CID expected;
  };

  struct ZerocommTest : ::testing::TestWithParam<Params> {};

  /**
   * Compare zero commitment with one from Lotus. Expected values are from
   * extern/sector-storage/zerocomm/zerocomm_test.go from commit
   * (d4fef1b5f82b3602a1ff45979ad035e67280e334).
   */
  TEST_P(ZerocommTest, getZeroCommitment) {
    auto &params = GetParam();
    EXPECT_OUTCOME_EQ(getZeroPieceCommitment(params.unpadded_size),
                      params.expected)
  }

  INSTANTIATE_TEST_SUITE_P(
      ZerocommTestCases,
      ZerocommTest,
      ::testing::Values(Params{1016,
                               "baga6ea4seaqb66wjlfkrbye6uqoemcyxmqylwmrm235ucl"
                               "wfpsyx3ge2imidoly"},
                        Params{2032,
                               "baga6ea4seaqpy7usqklokfx2vxuynmupslkeutzexe2uqu"
                               "rdg5vhtebhxqmpqmy"},
                        Params{4064,
                               "baga6ea4seaqarrd3hdxbhpcd6qnzcxao5wmrditaq2z62y"
                               "sadp45lc4ndhp7mja"},
                        Params{8128,
                               "baga6ea4seaqlfzd37mi7vtmud5rk6xdvb47kltcn6ul5lr"
                               "hrnwzljv33v3a2gly"}));

}  // namespace fc::sector_storage::zerocomm
