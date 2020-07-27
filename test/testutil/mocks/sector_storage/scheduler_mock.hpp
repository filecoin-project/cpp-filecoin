/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SCHEDULER_MOCK_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SCHEDULER_MOCK_HPP

#include <gmock/gmock.h>

#include "sector_storage/scheduler.hpp"

namespace fc::sector_storage {

  class SchedulerMock : public Scheduler {
   public:
    MOCK_METHOD6(schedule,
                 outcome::result<void>(const SectorId &,
                                       const TaskType &,
                                       const std::shared_ptr<WorkerSelector> &,
                                       const WorkerAction &,
                                       const WorkerAction &,
                                       uint64_t));

    MOCK_METHOD1(doNewWorker, void(WorkerHandle *));
    void newWorker(std::unique_ptr<WorkerHandle> worker) {
      doNewWorker(worker.get());
    }

    MOCK_CONST_METHOD0(getSealProofType, RegisteredProof());
  };

}  // namespace fc::sector_storage

#endif  // CPP_FILECOIN_TEST_TESTUTIL_MOCKS_SECTOR_STORAGE_SCHEDULER_MOCK_HPP
