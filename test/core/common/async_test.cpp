/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/async.hpp"
#include <gtest/gtest.h>
#include <mutex>
#include <thread>

namespace fc {

  void expectResultContains(std::vector<std::pair<int, int>> res,
                            int context,
                            int result) {
    EXPECT_TRUE(
        std::find(res.begin(), res.end(), std::make_pair(context, result))
        != res.end());
  }

  void call(int i, const std::function<void(int)> &cb) {
    cb(i * 3);
  }

  /**
   * @given AsyncWaiter called twice
   * @when finalize called to expect final result
   * @then final callback called on finalize()
   */
  TEST(AsyncWaiterTest, SimpleTest) {
    bool final_called = false;
    auto waiter =
        std::make_shared<AsyncWaiter<int, int>>(2, [&final_called](auto res) {
          // check res
          EXPECT_EQ(res.size(), 2);
          expectResultContains(res, 1, 3);
          expectResultContains(res, 2, 6);
          final_called = true;
        });
    call(1, waiter->on(1));
    EXPECT_FALSE(final_called);
    call(2, waiter->on(2));
    EXPECT_TRUE(final_called);
  }
}  // namespace fc
