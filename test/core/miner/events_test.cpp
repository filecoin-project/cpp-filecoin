/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/events_impl.hpp"

#include "testutil/mocks/miner/tipset_cache_mock.hpp"
#include "testutil/outcome.hpp"

namespace fc::mining {

  using primitives::block::BlockHeader;

  class EventsTest : public ::testing::Test {
   protected:
    virtual void SetUp() {
      channel_ = std::make_shared<Channel<std::vector<HeadChange>>>();

      api_ = std::make_shared<FullNodeApi>();

      api_->ChainNotify = [&]() { return channel_; };

      tipset_cache_ = std::make_shared<TipsetCacheMock>();

      auto maybe_events = EventsImpl::createEvents(api_, tipset_cache_);
      ASSERT_FALSE(maybe_events.has_error());
      events_ = std::move(maybe_events.value());
    }

    std::shared_ptr<Channel<std::vector<HeadChange>>> channel_;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<TipsetCacheMock> tipset_cache_;
    std::shared_ptr<Events> events_;
  };

  /**
   * @given events
   * @when try to add Handler, but tipsetcache cannot get tipset
   * @then error occurs
   */
  TEST_F(EventsTest, ChainAtNotFoundTipset) {
    Events::HeightHandler height_handler;
    Events::RevertHandler revert_handler;
    EpochDuration confidence = 4;
    ChainEpoch height = 4;

    const auto error = ERROR_TEXT("API_ERROR");
    EXPECT_CALL(*tipset_cache_, best()).WillOnce(testing::Return(error));

    EXPECT_OUTCOME_ERROR(
        error,
        events_->chainAt(height_handler, revert_handler, confidence, height));
  }

  /**
   * @given events, tipset that should be executed immediately
   * @when try to add Handler, but tipsetcache cannot get tipset in second time
   * @then error occurs
   */
  TEST_F(EventsTest, ChainAtNotFoundTipsetAfterExecution) {
    Events::HeightHandler height_handler =
        [](const Tipset &, ChainEpoch) -> outcome::result<void> {
      return outcome::success();
    };
    Events::RevertHandler revert_handler;
    EpochDuration confidence = 4;
    ChainEpoch height = 4;

    const auto error = ERROR_TEXT("API_ERROR");
    BlockHeader block;
    block.height = 9;
    Tipset tipset(TipsetKey(), {block});
    EXPECT_CALL(*tipset_cache_, best())
        .WillOnce(testing::Return(tipset))
        .WillOnce(testing::Return(error));

    EXPECT_CALL(*tipset_cache_, getNonNull(height))
        .WillOnce(testing::Return(outcome::success(tipset)));

    EXPECT_OUTCOME_ERROR(
        error,
        events_->chainAt(height_handler, revert_handler, confidence, height));
  }

  /**
   * @given events, tipset that should be executed immediately
   * @when try to add Handler
   * @then Handler is called
   */
  TEST_F(EventsTest, ChainAtWithGlobalConfidence) {
    bool is_called = false;
    Events::HeightHandler height_handler =
        [&](const Tipset &, ChainEpoch) -> outcome::result<void> {
      is_called = true;
      return outcome::success();
    };
    Events::RevertHandler revert_handler;
    EpochDuration confidence = 4;
    ChainEpoch height = 4;
    BlockHeader block;
    block.height = height + confidence + kGlobalChainConfidence;
    Tipset tipset(TipsetKey(), {block});
    EXPECT_CALL(*tipset_cache_, best())
        .WillOnce(testing::Return(tipset))
        .WillOnce(testing::Return(tipset));

    EXPECT_CALL(*tipset_cache_, getNonNull(height))
        .WillOnce(testing::Return(outcome::success(tipset)));

    EXPECT_OUTCOME_TRUE_1(
        events_->chainAt(height_handler, revert_handler, confidence, height));
    EXPECT_TRUE(is_called);
  }

  /**
   * @given events, tipset
   * @when try to add Handler, then Apply and Revert tipset
   * @then success
   */
  TEST_F(EventsTest, ChainAtAddHandler) {
    bool apply_called = false;
    Events::HeightHandler height_handler =
        [&](const Tipset &, ChainEpoch) -> outcome::result<void> {
      apply_called = true;
      return outcome::success();
    };
    bool revert_called = false;
    Events::RevertHandler revert_handler = [&](const Tipset &) {
      revert_called = true;
      return outcome::success();
    };
    EpochDuration confidence = 1;
    ChainEpoch height = 4;
    BlockHeader block;
    block.height = height;
    std::shared_ptr<Tipset> tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<BlockHeader>({block}));
    EXPECT_CALL(*tipset_cache_, best()).WillOnce(testing::Return(*tipset));

    EXPECT_OUTCOME_TRUE_1(
        events_->chainAt(height_handler, revert_handler, confidence, height));
    EXPECT_FALSE(apply_called);

    BlockHeader block1;
    block1.height = height + confidence;
    std::shared_ptr<Tipset> tipset1 = std::make_shared<Tipset>(
        TipsetKey(), std::vector<BlockHeader>({block1}));

    EXPECT_CALL(*tipset_cache_, add(*tipset1))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*tipset_cache_, getNonNull(height))
        .WillOnce(testing::Return(outcome::success(*tipset)));

    EXPECT_CALL(*tipset_cache_, get(block1.height - 1))
        .WillOnce(testing::Return(outcome::success(*tipset)));
    // result should be some value
    // it means, that previous block already applied

    HeadChange change{.type = HeadChangeType::APPLY, .value = tipset1};
    channel_->write({change});
    EXPECT_TRUE(apply_called);

    EXPECT_CALL(*tipset_cache_, get(block.height))
        .WillOnce(testing::Return(outcome::success(*tipset)));

    EXPECT_CALL(*tipset_cache_, get(block.height - 1))
        .WillOnce(testing::Return(outcome::success(*tipset)));
    // result should be some value
    // it means, that previous block already applied

    EXPECT_CALL(*tipset_cache_, revert(*tipset1))
        .WillOnce(testing::Return(outcome::success()));
    EXPECT_CALL(*tipset_cache_, revert(*tipset))
        .WillOnce(testing::Return(outcome::success()));

    HeadChange revert_change1{.type = HeadChangeType::REVERT, .value = tipset1};
    HeadChange revert_change2{.type = HeadChangeType::REVERT, .value = tipset};
    channel_->write({revert_change1, revert_change2});
    EXPECT_TRUE(revert_called);
  }

  /**
   * @given events, base tipset and tipset
   * @when try to add Handler, then Apply and Revert tipset(also for all blanks)
   * @then success
   */
  TEST_F(EventsTest, ChainAtAddHandlerWithMissingTipset) {
    bool apply_called1 = false;
    Events::HeightHandler height_handler1 =
        [&](const Tipset &, ChainEpoch) -> outcome::result<void> {
      apply_called1 = true;
      return outcome::success();
    };
    bool revert_called1 = false;
    Events::RevertHandler revert_handler1 = [&](const Tipset &) {
      revert_called1 = true;
      return outcome::success();
    };
    EpochDuration confidence1 = 1;
    ChainEpoch height1 = 6;

    BlockHeader block;
    block.height = height1 - 1;
    std::shared_ptr<Tipset> tipset = std::make_shared<Tipset>(
        TipsetKey(), std::vector<BlockHeader>({block}));

    EXPECT_CALL(*tipset_cache_, best()).WillOnce(testing::Return(*tipset));

    EXPECT_OUTCOME_TRUE_1(events_->chainAt(
        height_handler1, revert_handler1, confidence1, height1));

    bool apply_called2 = false;
    Events::HeightHandler height_handler2 =
        [&](const Tipset &, ChainEpoch) -> outcome::result<void> {
      apply_called2 = true;
      return outcome::success();
    };
    bool revert_called2 = false;
    Events::RevertHandler revert_handler2 = [&](const Tipset &) {
      revert_called2 = true;
      return outcome::success();
    };
    EpochDuration confidence2 = 1;
    ChainEpoch height2 = 7;

    EXPECT_CALL(*tipset_cache_, best()).WillOnce(testing::Return(*tipset));

    EXPECT_OUTCOME_TRUE_1(events_->chainAt(
        height_handler2, revert_handler2, confidence2, height2));

    BlockHeader block3;
    block3.height = height2 + confidence2;
    std::shared_ptr<Tipset> tipset3 = std::make_shared<Tipset>(
        TipsetKey(), std::vector<BlockHeader>({block3}));

    EXPECT_CALL(*tipset_cache_, add(*tipset3))
        .WillOnce(testing::Return(outcome::success()));

    EXPECT_CALL(*tipset_cache_, getNonNull(block3.height - 1))
        .WillOnce(testing::Return(outcome::success(*tipset3)));

    EXPECT_CALL(*tipset_cache_, get(block3.height - 1))
        .WillOnce(testing::Return(outcome::success(boost::none)));

    EXPECT_CALL(*tipset_cache_, getNonNull(block3.height - 2))
        .WillOnce(testing::Return(outcome::success(*tipset3)));

    EXPECT_CALL(*tipset_cache_, get(block3.height - 2))
        .WillOnce(testing::Return(outcome::success(boost::none)));

    EXPECT_CALL(*tipset_cache_, get(block.height))
        .WillOnce(testing::Return(outcome::success(*tipset)));
    // result should be some value
    // it means, that previous block already applied

    HeadChange change{.type = HeadChangeType::APPLY, .value = tipset3};
    channel_->write({change});
    EXPECT_TRUE(apply_called1);
    EXPECT_TRUE(apply_called2);

    EXPECT_CALL(*tipset_cache_, get(block3.height - 1))
        .WillOnce(testing::Return(outcome::success(boost::none)));

    EXPECT_CALL(*tipset_cache_, get(block3.height - 2))
        .WillOnce(testing::Return(outcome::success(boost::none)));

    EXPECT_CALL(*tipset_cache_, get(block.height))
        .WillOnce(testing::Return(outcome::success(*tipset)));
    // result should be some value
    // it means, that previous block already applied

    EXPECT_CALL(*tipset_cache_, revert(*tipset3))
        .WillOnce(testing::Return(outcome::success()));

    HeadChange revert_change{.type = HeadChangeType::REVERT, .value = tipset3};
    channel_->write({revert_change});
    EXPECT_TRUE(revert_called1);
    EXPECT_TRUE(revert_called2);
  }
}  // namespace fc::mining
