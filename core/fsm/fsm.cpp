/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsm/fsm.hpp"

#include <iostream>
#include <vector>

#include <boost/asio.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

namespace fc::fsm {}

using namespace fc::fsm;

int main() {
  enum class Events { START, STOP, FAILURE };

  enum class States {
    READY,
    WORKING,
    STOPPED,
    FAULTY,
  };

  struct SomeStatableType {
    int counter;
    std::string name;
    States current_status;
  };

  using E = Transition<Events, States, SomeStatableType>;

  std::vector events = {E(Events::START)
                            .from(States::READY)
                            .to(States::WORKING)
                            .from(States::WORKING)
                            .toSameState(),
                        E(Events::FAILURE)
                            .fromAny()
                            .to(States::FAULTY)
                            .action([](auto ent, auto, auto, auto final_state) {
                              std::cout << "EVENT TRANSITION" << std::endl;
                            }),
                        E(Events::STOP)
                            .fromMany(States::READY, States::WORKING)
                            .to(States::STOPPED)};

  auto io = std::make_shared<boost::asio::io_context>();
  auto sch = std::make_shared<libp2p::protocol::AsioScheduler>(
      *io, libp2p::protocol::SchedulerConfig{50});

  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
      [&io](const boost::system::error_code &, int) { io->stop(); });

  FSM<Events, States, SomeStatableType> fsm(events, sch);

  fsm.setAnyChangeAction([](auto ent, auto, auto, auto final_state) {
    std::cout << "TRANSITION" << std::endl;
    ent->current_status = final_state;
  });

  auto ent = std::make_shared<SomeStatableType>();

  io->post([&]() { fsm.begin(ent, States::READY); });
  io->post([&]() { fsm.send(ent, Events::FAILURE); });

  io->run_for(std::chrono::seconds(5));
  std::cout << static_cast<int>(ent->current_status == States::FAULTY)
            << std::endl;

  std::cout << events.size() << std::endl;
  return 0;
}
