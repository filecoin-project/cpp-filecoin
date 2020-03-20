/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/blockchain/sync_manager/sync_target_bucket_test.hpp"

/** bucket is equal to itself */
TEST_F(SyncTargetBucketTest, EqualBucketsSuccess) {
  ASSERT_EQ(bucket2, bucket2);
}

/** bucket is not equal to another bucket */
TEST_F(SyncTargetBucketTest, EqualBucketsFail) {
  ASSERT_FALSE(bucket1 == bucket2);
}

/** bucket contains one of its tipsets */
TEST_F(SyncTargetBucketTest, IsSameChainSuccess) {
  EXPECT_OUTCOME_TRUE(res, bucket2.isSameChain(tipset2));
  ASSERT_TRUE(res);
}

/** bucket doesn't contain some tipset */
TEST_F(SyncTargetBucketTest, IsSameChainFail) {
  EXPECT_OUTCOME_TRUE(res, bucket1.isSameChain(tipset2));
  ASSERT_FALSE(res);
}

/** get heaviest tipset and ensure it is expected one */
TEST_F(SyncTargetBucketTest, GetHeaviestSuccess) {
  auto ts = bucket2.getHeaviestTipset();
  ASSERT_NE(ts, boost::none);
  ASSERT_EQ(*ts, tipset2);
}

TEST_F(SyncTargetBucketTest, GetHeaviestFail) {
  auto err = empty_bucket.getHeaviestTipset();
  ASSERT_EQ(err, boost::none);
}
