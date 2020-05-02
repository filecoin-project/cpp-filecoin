/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/fsm.hpp"

#include <gtest/gtest.h>

#include "host/context/impl/host_context_impl.hpp"
#include "testutil/outcome.hpp"

using HostContext = fc::host::HostContextImpl;

enum class Events { START, STOP };

enum class States { READY, WORKING, STOPPED };

struct Data {
  int x{};
  std::string content{};
};

using Fsm = fc::fsm::FSM<Events, States, Data>;
using Transition = Fsm::TransitionRule;

TEST(Dev, Main) {
  auto context = std::make_shared<HostContext>();
  Fsm fsm{{Transition(Events::START)
               .from(States::READY)
               .to(States::WORKING)
               .action([](auto data, auto, auto, auto) {
                 data->x = 1;
                 ASSERT_NE(data->content, "stopped");
               }),
           Transition(Events::STOP)
               .from(States::WORKING)
               .to(States::STOPPED)
               .action([](auto data, auto, auto, auto) {
                 ASSERT_EQ(data->x, 1);
                 data->content = "stopped";
               })},
          context};
  auto entity = std::make_shared<Data>();
  EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
  EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START))
  EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::STOP))
  context->runIoContext(2);
  ASSERT_EQ(entity->x, 1);
  ASSERT_EQ(entity->content, "stopped");
}
