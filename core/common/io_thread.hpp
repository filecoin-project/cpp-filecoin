/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <thread>

namespace fc {
  struct IoThread {
    inline IoThread()
        : io{std::make_shared<boost::asio::io_context>()},
          work{io->get_executor()},
          thread{[this] { io->run(); }} {}
    inline ~IoThread() {
      io->stop();
      if (thread.joinable()) {
        thread.join();
      }
    }

    std::shared_ptr<boost::asio::io_context> io;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work;
    std::thread thread;
  };
}  // namespace fc
