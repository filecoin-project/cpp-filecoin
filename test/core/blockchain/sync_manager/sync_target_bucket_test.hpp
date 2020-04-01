/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_CORE_BLOCKCHAIN_SYNC_MANAGER_SYNC_TARGET_BUCKET_TEST_HPP
#define CPP_FILECOIN_TEST_CORE_BLOCKCHAIN_SYNC_MANAGER_SYNC_TARGET_BUCKET_TEST_HPP

#include "blockchain/impl/sync_target_bucket.hpp"

#include <gtest/gtest.h>

#include "common/hexutil.hpp"
#include "common/outcome.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

struct SyncTargetBucketTest : public ::testing::Test {
  using Tipset = fc::primitives::tipset::Tipset;
  using TipsetKey = fc::primitives::tipset::TipsetKey;
  using SyncTargetBucket = fc::blockchain::sync_manager::SyncTargetBucket;
  using SyncTargetBucketError =
      fc::blockchain::sync_manager::SyncTargetBucketError;
  using BlockHeader = fc::primitives::block::BlockHeader;
  using Address = fc::primitives::address::Address;
  using BigInt = fc::primitives::BigInt;
  using CID = fc::CID;
  using Signature = fc::primitives::block::Signature;
  using Ticket = fc::primitives::ticket::Ticket;
  using TipsetError = fc::primitives::tipset::TipsetError;

  ~SyncTargetBucketTest() override = default;

  BlockHeader makeBlock1() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto bls2 =
        "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto ticket2 = Ticket{bls2};
    BlockHeader block_header = fc::primitives::block::BlockHeader{
        fc::primitives::address::Address::makeFromId(1),
        ticket2,
        {
            fc::common::Buffer("F00D"_unhex),
            bls1,
            {},
        },
        {"010001020002"_cid},
        BigInt(3),
        4,
        "010001020005"_cid,
        "010001020006"_cid,
        "010001020007"_cid,
        "CAFE"_unhex,
        8,
        Signature{"DEAD"_unhex},
        9,
    };
    return block_header;
  }

  BlockHeader makeBlock2() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto ticket2 = Ticket{bls1};
    BlockHeader block_header = fc::primitives::block::BlockHeader{
        fc::primitives::address::Address::makeFromId(2),
        ticket2,
        {
            fc::common::Buffer("F00D"_unhex),
            bls1,
            {},
        },
        {"010001020002"_cid},
        BigInt(4),
        4,
        "010001020005"_cid,
        "010001020006"_cid,
        "010001020007"_cid,
        "CAFE"_unhex,
        8,
        Signature{"DEAD"_unhex},
        9,
    };
    return block_header;
  }

  void SetUp() override {
    bh1 = makeBlock1();
    bh2 = makeBlock2();
    EXPECT_OUTCOME_TRUE(ts1, Tipset::create({bh1}));
    EXPECT_OUTCOME_TRUE(ts2, Tipset::create({bh1, bh2}));
    tipset1 = std::move(ts1);
    tipset2 = std::move(ts2);
    bucket1 = {{tipset1}};
    bucket2 = {{tipset1, tipset2}};
  }

  BlockHeader bh1;
  BlockHeader bh2;
  Tipset tipset1;
  Tipset tipset2;
  SyncTargetBucket bucket1;
  SyncTargetBucket bucket2;
  SyncTargetBucket empty_bucket{};
};

#endif  // CPP_FILECOIN_TEST_CORE_BLOCKCHAIN_SYNC_MANAGER_SYNC_TARGET_BUCKET_TEST_HPP
