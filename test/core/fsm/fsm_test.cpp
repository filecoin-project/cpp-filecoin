/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/fsm.hpp"

#include <gtest/gtest.h>
#include <string>

#include "testutil/outcome.hpp"

namespace fc::fsm {
  enum class Events { START, STOP };

  struct EventContext {
    EventContext(int mult, const std::string msg)
        : multiplier(mult), message(msg){};

    int multiplier;
    std::string message;
  };

  enum class States { READY, WORKING, STOPPED };

  struct Data {
    int x{};
    std::string content{};
  };

  using Fsm = fc::fsm::FSM<Events, EventContext, States, Data>;
  using TransitionRule = Fsm::TransitionRule;

  class FsmTest : public ::testing::Test {
   public:
    boost::asio::io_context io_context;
  };

  /**
   * Test pipeline with context change
   */
  TEST_F(FsmTest, Main) {
    Fsm fsm{{TransitionRule(Events::START)
                 .from(States::READY)
                 .to(States::WORKING)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   data->x = ctx->multiplier;
                   ASSERT_NE(data->content, "stopped");
                 }),
             TransitionRule(Events::STOP)
                 .from(States::WORKING)
                 .to(States::STOPPED)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   ASSERT_EQ(data->x, 1);
                   data->x *= ctx->multiplier;
                   data->content = "stopped";
                 })},
            io_context,
            true};
    auto entity = std::make_shared<Data>();
    fsm.setAnyChangeAction([](auto entity, auto, auto ctx, auto, auto) {
      entity->content = entity->content + std::string(" after ") + ctx->message;
    });
    EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
    EXPECT_OUTCOME_TRUE_1(fsm.send(
        entity, Events::START, std::make_shared<EventContext>(1, "starting")))
    EXPECT_OUTCOME_TRUE_1(fsm.send(
        entity, Events::STOP, std::make_shared<EventContext>(2, "stopping")))
    for (auto i{0}; i < 2; ++i) {
      io_context.run_one();
    }
    EXPECT_EQ(entity->x, 2);
    EXPECT_EQ(entity->content, "stopped after stopping");
    EXPECT_OUTCOME_TRUE_1(fsm.force(entity, States::WORKING));
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
  }

  /**
   * @given events sent in reverse order (STOP, START), so it cannot be executed
   * in the initial order and FSM discards such events
   * @when execute
   * @then the first event (STOP) discarded, then START event executed
   */
  TEST_F(FsmTest, SendBeforeConditionMetAndDiscard) {
    boost::asio::io_context io_context;

    Fsm fsm{{TransitionRule(Events::START)
                 .from(States::READY)
                 .to(States::WORKING)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   data->content = "working";
                 }),
             TransitionRule(Events::STOP)
                 .from(States::WORKING)
                 .to(States::STOPPED)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   data->content = "stopped";
                 })},
            io_context,
            true};
    auto entity = std::make_shared<Data>(Data{0, "ready"});

    EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::STOP, {}));
    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START, {}));

    // initial state
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::READY);
    ASSERT_EQ(entity->content, "ready");

    // discard STOP
    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::READY);
    ASSERT_EQ(entity->content, "ready");

    // serve START
    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
    ASSERT_EQ(entity->content, "working");
  }

  /**
   * @given events sent in reverse order (STOP, START), so it cannot be executed
   * in the initial order
   * @when execute
   * @then the first event (STOP) rescheduled, then START event executed, then
   * postponed STOP executed
   */
  TEST_F(FsmTest, SendBeforeConditionMet) {
    Fsm fsm{{TransitionRule(Events::START)
                 .from(States::READY)
                 .to(States::WORKING)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   data->content = "working";
                 }),
             TransitionRule(Events::STOP)
                 .from(States::WORKING)
                 .to(States::STOPPED)
                 .action([](auto data, auto, auto ctx, auto, auto) {
                   data->content = "stopped";
                 })},
            io_context,
            false};
    auto entity = std::make_shared<Data>(Data{0, "ready"});

    EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::STOP, {}));
    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START, {}));

    // initial state
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::READY);
    ASSERT_EQ(entity->content, "ready");

    // reschedule STOP event
    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::READY);
    ASSERT_EQ(entity->content, "ready");

    // serve START
    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
    ASSERT_EQ(entity->content, "working");

    // serve rescheduled STOP
    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::STOPPED);
    ASSERT_EQ(entity->content, "stopped");
  }

  /**
   * @given FSM can process rules with the same event and different initial
   * states
   * @when event is scheduled
   * @then it is processed and not skipped
   */
  TEST_F(FsmTest, EventRedefined) {
    Fsm fsm{{
                TransitionRule(Events::START)
                    .from(States::READY)
                    .to(States::WORKING)
                    .action([](auto data, auto, auto ctx, auto, auto) {
                      data->content = "working";
                    }),
                TransitionRule(Events::START)
                    .from(States::WORKING)
                    .to(States::WORKING)
                    .action([](auto data, auto, auto ctx, auto, auto) {
                      data->content = "still working";
                    }),
            },

            io_context,
            false};

    auto entity = std::make_shared<Data>(Data{0, "ready"});

    EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::READY);
    ASSERT_EQ(entity->content, "ready");

    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START, {}));
    EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START, {}));

    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
    ASSERT_EQ(entity->content, "working");

    io_context.run_one();
    EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
    ASSERT_EQ(entity->content, "still working");
  }

}  // namespace fc::fsm
