/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/sectors.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using storage::amt::AmtError;
  using storage::ipfs::InMemoryDatastore;

  struct SectorsTest : testing::Test {
    void SetUp() override {
      ipld->load(setup_sectors);

      EXPECT_OUTCOME_TRUE_1(
          setup_sectors.store({makeSector(0), makeSector(1), makeSector(5)}));
    }

    SectorOnChainInfo makeSector(SectorNumber number) const {
      SectorOnChainInfo sector;
      sector.sector = number;
      sector.seal_proof = RegisteredSealProof::kStackedDrg32GiBV1_1;
      sector.sealed_cid = kEmptyObjectCid;

      return sector;
    }

    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};

    Sectors setup_sectors;
  };

  TEST_F(SectorsTest, LoadsSectors) {
    EXPECT_OUTCOME_TRUE(sectors, setup_sectors.load({0, 5}));
    EXPECT_EQ(sectors.size(), 2);
    EXPECT_EQ(sectors[0], makeSector(0));
    EXPECT_EQ(sectors[1], makeSector(5));

    EXPECT_OUTCOME_ERROR(AmtError::kNotFound, setup_sectors.load({0, 3}));
  }

  TEST_F(SectorsTest, StoresSectors) {
    const auto s0 = makeSector(0);
    auto s1 = makeSector(1);
    s1.activation_epoch = 1;
    const auto s3 = makeSector(3);
    const auto s5 = makeSector(5);

    EXPECT_OUTCOME_TRUE_1(setup_sectors.store({s3, s1}));
    EXPECT_OUTCOME_TRUE(sectors, setup_sectors.load({0, 1, 3, 5}));

    EXPECT_EQ(sectors.size(), 4);
    EXPECT_EQ(sectors[0], s0);
    EXPECT_EQ(sectors[1], s1);
    EXPECT_EQ(sectors[2], s3);
    EXPECT_EQ(sectors[3], s5);
  }

  TEST_F(SectorsTest, LoadsAndStoresNoSectors) {
    EXPECT_OUTCOME_TRUE(sectors, setup_sectors.load({}));
    EXPECT_TRUE(sectors.empty());
    EXPECT_OUTCOME_TRUE_1(setup_sectors.store({}));
  }

  TEST_F(SectorsTest, LoadsForProofWithReplacement) {
    const auto s1 = makeSector(1);
    EXPECT_OUTCOME_TRUE(infos, setup_sectors.loadForProof({0, 1}, {0}));
    const std::vector<SectorOnChainInfo> expected_infos{s1, s1};
    EXPECT_EQ(infos, expected_infos);
  }

  TEST_F(SectorsTest, LoadsForProofWithoutReplacement) {
    const auto s0 = makeSector(0);
    const auto s1 = makeSector(1);
    EXPECT_OUTCOME_TRUE(infos, setup_sectors.loadForProof({0, 1}, {}));
    const std::vector<SectorOnChainInfo> expected_infos{s0, s1};
    EXPECT_EQ(infos, expected_infos);
  }

  TEST_F(SectorsTest, EmptyProof) {
    EXPECT_OUTCOME_TRUE(infos, setup_sectors.loadForProof({}, {}));
    EXPECT_TRUE(infos.empty());
  }

  TEST_F(SectorsTest, NoNonfaultySectors) {
    EXPECT_OUTCOME_TRUE(infos, setup_sectors.loadForProof({1}, {1}));
    EXPECT_TRUE(infos.empty());
  }

}  // namespace fc::vm::actor::builtin::types::miner
