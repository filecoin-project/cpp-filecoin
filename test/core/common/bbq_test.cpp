/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/bbq.hpp"

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""s;

struct BbqTest : public ::testing::Test {
 public:
  //  void SetUp() override {}
  //  boost::asio::io_service service;
};

TEST_F(BbqTest, SingleConsumerProdecerSuccess) {
  auto bbq = std::make_shared<fc::common::BufferedBlockingQueue<int>>(1);

  std::thread producer([tx = bbq->getTransmitter()]() {
    auto t = tx.lock();
    if (!t) {
      std::cout << "transmitter is not available, quit" << std::endl;
      return;
    }
    for (size_t i = 0; i < 3; ++i) {
      if (!t->push(i)) {
        std::cout << "push failed, channel is closed" << std::endl;
        return;
      }
      std::cout << "pushed " << i << std::endl;
    }
  });

  std::thread consumer([rx = bbq->getReceiver()]() {
    auto r = rx.lock();
    if (!r) {
      std::cout << "receiver is not available, quit" << std::endl;
      return;
    }

    while (true) {
      auto &&v = r->pop();
      if (!v.is_initialized()) {
        std::cout << "pop failed, channel is closing" << std::endl;
        return;
      }
      std::cout << "pop " << v.value() << std::endl;
    }
  });

  std::this_thread::sleep_for(200ms);

  bbq->close();

  if (consumer.joinable()) consumer.join();
  if (producer.joinable()) producer.join();
}
