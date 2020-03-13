/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/rx.hpp"

#include <gtest/gtest.h>

using namespace fc::common;

void one_thread() {
  std::set<int> received;
  std::set<int> sent;
  int count = 0;

  auto io = std::make_shared<boost::asio::io_context>();
  const auto period = boost::posix_time::milliseconds(2);
  boost::asio::deadline_timer timer(*io, period);

  auto rx = std::make_shared<RX<int>>(io, [&io, &received](int msg) {
    received.insert(msg);
    if (received.size() == 100) {
      io->stop();
    }
  });

  auto tx = rx->get_tx();

  auto send_fn = [&]() {
    if (++count > 100) {
      return;
    } else {
      tx.send(count);
      sent.insert(count);
    }
  };

  std::function<void(const boost::system::error_code &)> timer_fn = [&](auto) {
    send_fn();
    timer.expires_from_now(period);
    timer.async_wait(timer_fn);
  };

  timer.async_wait(timer_fn);

  io->run();

  assert(received.size() == 100);
  assert(sent == received);
}

void two_threads() {
  std::set<int> received;
  std::set<int> sent;
  int count = 0;

  auto io = std::make_shared<boost::asio::io_context>();

  auto rx = std::make_shared<RX<int>>(io, [&io, &received](int msg) {
    received.insert(msg);
    if (received.size() == 100) {
      io->stop();
    }
  });

  auto tx = rx->get_tx();

  auto send_fn = [&]() {
    if (++count > 100) {
      return;
    } else {
      tx.send(count);
      sent.insert(count);
    }
  };

  auto f = std::async(std::launch::async, [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(666));
    for (int i = 1; i <= 100; ++i) {
      send_fn();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  });

  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
      [&io](const boost::system::error_code &, int) { io->stop(); });

  io->run();

  f.get();

  assert(received.size() == 100);
  assert(sent == received);
}

TEST(RxTest, RxSuccess) {
  one_thread();
  two_threads();
}
