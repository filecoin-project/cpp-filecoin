/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/request_queue.hpp"

#include <gtest/gtest.h>

using fc::sector_storage::RequestQueue;
using fc::sector_storage::WorkerRequest;

class RequestQueueTest : public ::testing::Test {
 public:
  void SetUp() override {
    requests_.push_back(WorkerRequest{
        .sector =
            {
                .miner = 42,
                .sector = 1,
            },
        .task_type = fc::primitives::kTTAddPiece,
        .priority = 0,
    });
    requests_.push_back(WorkerRequest{
        .sector =
            {
                .miner = 42,
                .sector = 1,
            },
        .task_type = fc::primitives::kTTPreCommit1,
        .priority = 0,
    });
    requests_.push_back(WorkerRequest{
        .sector =
            {
                .miner = 42,
                .sector = 1,
            },
        .task_type = fc::primitives::kTTPreCommit2,
        .priority = 0,
    });

    request_queue_ = std::make_shared<RequestQueue>();
  }

 protected:
  std::vector<WorkerRequest> requests_;

  std::shared_ptr<RequestQueue> request_queue_;
};

/**
 * @given queue and 3 requests
 * @when added 3 requests and pop 2
 * @then first is kTTPreCommit2 and second is kTTPreCommit1
 */
TEST_F(RequestQueueTest, Order) {
  for (const auto &request : requests_) {
    request_queue_->insert(request);
  }

  auto res1 = request_queue_->pop();
  ASSERT_TRUE(res1);
  ASSERT_EQ((*res1).task_type, fc::primitives::kTTPreCommit2);

  auto res2 = request_queue_->pop();
  ASSERT_TRUE(res2);
  ASSERT_EQ((*res2).task_type, fc::primitives::kTTPreCommit1);
}

/**
 * @given queue and 3 requests
 * @when added 3 requests, remove request with index 2(kTTPreCommit1) and pop 2
 * @then first is kTTPreCommit2 and second is kTTAddPiece
 */
TEST_F(RequestQueueTest, Remove) {
  for (const auto &request : requests_) {
    request_queue_->insert(request);
  }

  // 2 is kTTPreCommit1
  ASSERT_TRUE(request_queue_->remove(2));

  auto res1 = request_queue_->pop();
  ASSERT_TRUE(res1);
  ASSERT_EQ((*res1).task_type, fc::primitives::kTTPreCommit2);

  auto res2 = request_queue_->pop();
  ASSERT_TRUE(res2);
  ASSERT_EQ((*res2).task_type, fc::primitives::kTTAddPiece);
}
