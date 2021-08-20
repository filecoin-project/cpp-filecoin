/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "sector_storage/scheduler.hpp"

namespace fc::sector_storage {

  class SchedulerMock : public Scheduler {
   public:
    MOCK_METHOD8(schedule,
                 outcome::result<void>(const SectorRef &,
                                       const TaskType &,
                                       const std::shared_ptr<WorkerSelector> &,
                                       const WorkerAction &,
                                       const WorkerAction &,
                                       const ReturnCb &,
                                       uint64_t,
                                       const boost::optional<WorkId> &));

    MOCK_METHOD1(doNewWorker, void(WorkerHandle *));
    void newWorker(std::unique_ptr<WorkerHandle> worker) override {
      doNewWorker(worker.get());
    }

    MOCK_METHOD2(returnResult,
                 outcome::result<void>(const CallId &, CallResult));
  };

}  // namespace fc::sector_storage
