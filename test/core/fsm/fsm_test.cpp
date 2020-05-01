/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/fsm.hpp"

#include <vector>

#include <gtest/gtest.h>
#include "host/context/impl/host_context_impl.hpp"
#include "testutil/outcome.hpp"

using fc::fsm::FSM;
using HostContext = fc::host::HostContextImpl;

enum class Events { START, STOP };

enum class States { READY, WORKING, STOPPED };

struct Data {
  int x{};
  std::string content{};
};

using Transition = fc::fsm::Transition<Events, States, Data>;

void startHandler(std::shared_ptr<Data> data,
                  Events e,
                  States previous,
                  States next) {
  data->x = 1;
}

void stopHandler(std::shared_ptr<Data> data,
                 Events e,
                 States previous,
                 States next) {
  data->content = "stopped";
}

TEST(Dev, Main) {
  auto context = std::make_shared<HostContext>();
  std::vector<Transition> transitions = {Transition(Events::START)
                                             .from(States::READY)
                                             .to(States::WORKING)
                                             .action(startHandler),
                                         Transition(Events::STOP)
                                             .from(States::WORKING)
                                             .to(States::STOPPED)
                                             .action(stopHandler)};
  fc::fsm::FSM<Events, States, Data> fsm(transitions, context);
  auto entity = std::make_shared<Data>();
  EXPECT_OUTCOME_TRUE_1(fsm.begin(entity, States::READY))
  EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::START))
  EXPECT_OUTCOME_TRUE_1(fsm.send(entity, Events::STOP))
  context->runIoContext(1);
  ASSERT_EQ(entity->x, 1);
  ASSERT_EQ(entity->content, "stopped");
}
