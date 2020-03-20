/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/sync_bucket_set.hpp"

#include "core/blockchain/sync_manager/sync_target_bucket_test.hpp"

using namespace fc::blockchain;

/** @brief we need this struct only to access the buckets */
struct SyncBucketSetMock : public sync_manager::SyncBucketSet {
  using SyncBucketSet::SyncBucketSet;

  auto &getBuckets() {
    return buckets_;
  }
};

struct SyncBucketSetTest : public SyncTargetBucketTest {
  using Tipset = fc::primitives::tipset::Tipset;
  using TipsetKey = fc::primitives::tipset::TipsetKey;
  using SyncTargetBucket = fc::blockchain::sync_manager::SyncTargetBucket;
  using SyncBucketSet = sync_manager::SyncBucketSet;
  using BlockHeader = fc::primitives::block::BlockHeader;
  using Address = fc::primitives::address::Address;
  using BigInt = fc::primitives::BigInt;
  using CID = fc::CID;
  using Signature = fc::primitives::block::Signature;
  using Ticket = fc::primitives::ticket::Ticket;
  using TipsetError = fc::primitives::tipset::TipsetError;

  void SetUp() override {
    SyncTargetBucketTest::SetUp();
    empty_bucket = SyncBucketSetMock(std::vector<Tipset>({}));
    bucket_set1 = SyncBucketSetMock(std::vector<Tipset>({tipset1}));
    bucket_set2 = SyncBucketSetMock(std::vector<Tipset>({tipset1, tipset2}));
  }

  boost::optional<SyncBucketSetMock> empty_bucket;
  boost::optional<SyncBucketSetMock> bucket_set1;
  boost::optional<SyncBucketSetMock> bucket_set2;
};

/** check insert tipset */
TEST_F(SyncBucketSetTest, InsertTipsetSuccess) {
  bucket_set1->insert(tipset2);
  auto &bs = bucket_set1->getBuckets();
  ASSERT_EQ(bs.size(), 1);
  ASSERT_EQ(bs[0].tipsets.size(), 2);
}

TEST_F(SyncBucketSetTest, AppendTipsetSuccess) {
  SyncTargetBucket b{{tipset1, tipset2}};
  auto &bs = bucket_set1->getBuckets();
  ASSERT_EQ(bs.size(), 1);
  bucket_set1->append(b);
  ASSERT_EQ(bs.size(), 2);
}

/** ensure remove bucket works */
TEST_F(SyncBucketSetTest, RemoveBucketSuccess) {
  bucket_set2->removeBucket(bucket2);
  ASSERT_TRUE(bucket_set2->isEmpty());
}

/** ensure isRelatedToAny works right */
TEST_F(SyncBucketSetTest, IsRelatedToAnySuccess) {
  EXPECT_OUTCOME_TRUE_1(bucket_set1->isRelatedToAny(tipset1));
  EXPECT_OUTCOME_TRUE(res,bucket_set1->isRelatedToAny(tipset2));
  ASSERT_FALSE(res);
  EXPECT_OUTCOME_TRUE_1(bucket_set2->isRelatedToAny(tipset1));
  EXPECT_OUTCOME_TRUE_1(bucket_set2->isRelatedToAny(tipset2));
}

/** ensure that get heaviest tipset works */
TEST_F(SyncBucketSetTest, GetHeaviestTipsetSuccess) {
  EXPECT_OUTCOME_TRUE(heaviest, bucket_set2->getHeaviestTipset());
  ASSERT_EQ(heaviest, tipset2);
}

/** ensure that pop succeeds */
TEST_F(SyncBucketSetTest, PopSuccess) {
  boost::optional<SyncTargetBucket> value = bucket_set2->pop();
  ASSERT_NE(value, boost::none);
  ASSERT_EQ(*value, bucket2);
  value = bucket_set2->pop();
  ASSERT_EQ(value, boost::none);
}

/** ensure that pop related succeeds */
TEST_F(SyncBucketSetTest, PopRelatedSuccess) {
  EXPECT_OUTCOME_TRUE(related_target, bucket_set2->popRelated(tipset2));
  ASSERT_EQ(related_target, bucket2);
}

/** ensure that pop unrelated fails */
TEST_F(SyncBucketSetTest, PopUnRelatedFail) {
  EXPECT_OUTCOME_TRUE(related_target, bucket_set1->popRelated(tipset2))
  ASSERT_EQ(related_target, boost::none);
}

/** check test on emptiness */
TEST_F(SyncBucketSetTest, IsEmptySuccess) {
  ASSERT_TRUE(empty_bucket->isEmpty());
  ASSERT_FALSE(bucket_set1->isEmpty());
  ASSERT_FALSE(bucket_set2->isEmpty());
}

/** check getSize */
TEST_F(SyncBucketSetTest, GetSizeSuccess) {
  ASSERT_EQ(empty_bucket->getSize(), 0);
  ASSERT_EQ(bucket_set1->getSize(), 1);
  ASSERT_EQ(bucket_set2->getSize(), 1);
}
