/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/sector_stat_impl.hpp"

#include <gtest/gtest.h>

namespace fc::mining {

  class SectorStatTest : public testing::Test {
   protected:
    virtual void SetUp() {
      sector_stat_ = std::make_shared<SectorStatImpl>();
    }

    std::shared_ptr<SectorStat> sector_stat_;
  };

  /**
   * @given 3 sectors in sealing state
   * @when 2 sector goes to Proving state, after 3 sector goes to Failed state
   * @then current sealing is 2
   */
  TEST_F(SectorStatTest, UpdateSector) {
    SectorId sector1{
        .miner = 42,
        .sector = 1,
    };
    SectorId sector2{
        .miner = 42,
        .sector = 2,
    };
    SectorId sector3{
        .miner = 42,
        .sector = 3,
    };

    sector_stat_->updateSector(sector1, SealingState::kPreCommit1);
    sector_stat_->updateSector(sector2, SealingState::kPreCommit1);
    sector_stat_->updateSector(sector3, SealingState::kPreCommit1);
    ASSERT_EQ(sector_stat_->currentSealing(), 3);
    sector_stat_->updateSector(sector2, SealingState::kProving);
    ASSERT_EQ(sector_stat_->currentSealing(), 2);
    sector_stat_->updateSector(sector3, SealingState::kFaulty);
    ASSERT_EQ(sector_stat_->currentSealing(), 2);
  }
}  // namespace fc::mining
