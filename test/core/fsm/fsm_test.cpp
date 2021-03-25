/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/fsm.hpp"

#include <string>

#include <gtest/gtest.h>

#include "host/context/impl/host_context_impl.hpp"
#include "testutil/outcome.hpp"

using HostContext = fc::host::HostContextImpl;

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
using Transition = Fsm::TransitionRule;

TEST(Dev, Main) {
  auto context = std::make_shared<HostContext>();
  Fsm fsm{{Transition(Events::START)
               .from(States::READY)
               .to(States::WORKING)
               .action([](auto data, auto, auto ctx, auto, auto) {
                 data->x = ctx->multiplier;
                 ASSERT_NE(data->content, "stopped");
               }),
           Transition(Events::STOP)
               .from(States::WORKING)
               .to(States::STOPPED)
               .action([](auto data, auto, auto ctx, auto, auto) {
                 ASSERT_EQ(data->x, 1);
                 data->x *= ctx->multiplier;
                 data->content = "stopped";
               })},
          context};
  auto entity = std::make_shared<Data>();
  fsm.setAnyChangeAction([](auto entity, auto, auto ctx, auto, auto) {
    entity->content = entity->content + std::string(" after ") + ctx->message;
  });
  EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
  EXPECT_OUTCOME_TRUE_1(fsm.send(
      entity, Events::START, std::make_shared<EventContext>(1, "starting")))
  EXPECT_OUTCOME_TRUE_1(fsm.send(
      entity, Events::STOP, std::make_shared<EventContext>(2, "stopping")))
  context->runIoContext(1);
  ASSERT_EQ(entity->x, 2);
  ASSERT_EQ(entity->content, "stopped after stopping");
  EXPECT_OUTCOME_TRUE_1(fsm.force(entity, States::WORKING));
  EXPECT_OUTCOME_EQ(fsm.get(entity), States::WORKING);
}
