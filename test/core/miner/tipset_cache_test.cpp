/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/tipset_cache_impl.hpp"

#include <gtest/gtest.h>

#include "testutil/outcome.hpp"

#include "testutil/mocks/api.hpp"

namespace fc::mining {

  using primitives::block::BlockHeader;

  class TipsetCacheTest : public testing::Test {
   protected:
    virtual void SetUp() {
      capability_ = 4;
      tipset_cache_ = std::make_shared<TipsetCacheImpl>(capability_, api_);
    }

    uint64_t capability_;
    std::shared_ptr<TipsetCache> tipset_cache_;
    std::shared_ptr<FullNodeApi> api_ = std::make_shared<FullNodeApi>();
    MOCK_API(api_, ChainHead);
    MOCK_API(api_, ChainGetTipSetByHeight);
  };

  /**
   * @given 2 tipsets(0 height, 1 height)
   * @when try to add 1st tipset and then 2nd
   * @then success and best is 2nd
   */
  TEST_F(TipsetCacheTest, Add) {
    BlockHeader block;
    block.height = 1;
    Tipset tipset1(TipsetKey(), {});
    Tipset tipset2(TipsetKey(), {block});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_EQ(tipset_cache_->best(), tipset2);
  }

  /**
   * @given 2 tipsets(0 height, 1 height)
   * @when try to add 2nd tipset and then 1st
   * @then kSmallerHeight error occurs
   */
  TEST_F(TipsetCacheTest, AddSmallerHeight) {
    BlockHeader block;
    block.height = 1;
    Tipset tipset1(TipsetKey(), {});
    Tipset tipset2(TipsetKey(), {block});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_OUTCOME_ERROR(TipsetCacheError::kSmallerHeight,
                         tipset_cache_->add(tipset1));
  }

  /**
   * @given empty cache
   * @when try to revert some tipset
   * @then success
   */
  TEST_F(TipsetCacheTest, RevertEmpty) {
    Tipset tipset1(TipsetKey(), {});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->revert(tipset1));
  }

  /**
   * @given 2 tipsets(0 height, 2 height) in cache
   * @when try to revert top tipset
   * @then success and best is 1st
   */
  TEST_F(TipsetCacheTest, Revert) {
    BlockHeader block;
    block.height = 2;
    Tipset tipset1(TipsetKey(), {});
    Tipset tipset2(TipsetKey(), {block});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->revert(tipset2));
    EXPECT_EQ(tipset_cache_->best(), tipset1);
  }

  /**
   * @given 2 tipsets(0 height, 1 height)
   * @when try to revert not top tipset
   * @then kNotMatchHead error occurs
   */
  TEST_F(TipsetCacheTest, RevertNotHead) {
    BlockHeader block;
    block.height = 2;
    Tipset tipset1(TipsetKey(), {});
    Tipset tipset2(TipsetKey(), {block});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_OUTCOME_ERROR(TipsetCacheError::kNotMatchHead,
                         tipset_cache_->revert(tipset1));
  }

  /**
   * @given 2 tipsets(0 height, 3 height) in cache
   * @when try to get 0 height tipset
   * @then tipset is getted
   */
  TEST_F(TipsetCacheTest, Get) {
    BlockHeader block1;
    block1.height = 0;
    BlockHeader block2;
    block2.height = 3;
    Tipset tipset1(TipsetKey(), {block1});
    Tipset tipset2(TipsetKey(), {block2});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_OUTCOME_EQ(tipset_cache_->get(0), tipset1);
  }

  /**
   * @given 1 tipsets(1 height) in cache
   * @when try to get more height tipset
   * @then kNotInCache error occurs
   */
  TEST_F(TipsetCacheTest, GetNotInCache) {
    BlockHeader block1;
    block1.height = 1;
    Tipset tipset1(TipsetKey(), {block1});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_ERROR(TipsetCacheError::kNotInCache, tipset_cache_->get(4));
  }

  /**
   * @given 1 tipsets(3 height)
   * @when try to get less height tipset
   * @then success and get function is called
   */
  TEST_F(TipsetCacheTest, GetLess) {
    BlockHeader block1;
    block1.height = 3;
    Tipset tipset1(TipsetKey(), {block1});
    BlockHeader block2;
    block2.height = 1;
    auto tipset2 = std::make_shared<Tipset>(TipsetKey(),
                                            std::vector<BlockHeader>({block2}));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_CALL(mock_ChainGetTipSetByHeight, Call(3, TipsetKey{})).WillOnce(testing::Return(tipset2));
    EXPECT_OUTCOME_EQ(tipset_cache_->get(1), *tipset2);
  }

  /**
   * @given empty cache
   * @when try to get tipset
   * @then success and get function is called
   */
  TEST_F(TipsetCacheTest, GetEmpty) {
    BlockHeader block1;
    block1.height = 1;
    auto tipset1 = std::make_shared<Tipset>(TipsetKey(),
                                            std::vector<BlockHeader>({block1}));
    EXPECT_CALL(mock_ChainGetTipSetByHeight, Call(3, TipsetKey{})).WillOnce(testing::Return(tipset1));
    EXPECT_OUTCOME_EQ(tipset_cache_->get(1), *tipset1);
  }

  /**
   * @given 2 tipsets(1 height, 3 height)
   * @when try to get NotNull tipset from 2 height
   * @then success and best is 2nd(3 height)
   */
  TEST_F(TipsetCacheTest, NotNull) {
    BlockHeader block1;
    block1.height = 1;
    BlockHeader block2;
    block2.height = 3;
    Tipset tipset1(TipsetKey(), {block1});
    Tipset tipset2(TipsetKey(), {block2});
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset1));
    EXPECT_OUTCOME_TRUE_1(tipset_cache_->add(tipset2));
    EXPECT_OUTCOME_EQ(tipset_cache_->getNonNull(2), tipset2);
  }

  TEST_F(TipsetCacheTest, EmptyCache){
    BlockHeader block1;
    block1.height = 1;
    auto tipset1 = std::make_shared<Tipset>(TipsetKey(),
                                            std::vector<BlockHeader>({block1}));
    EXPECT_CALL(mock_ChainHead, Call()).WillOnce(testing::Return(tipset1));
    EXPECT_OUTCOME_EQ(tipset_cache_->best(), tipset1);

  }
}  // namespace fc::mining
