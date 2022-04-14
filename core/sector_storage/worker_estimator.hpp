/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/seal_tasks/task.hpp"
#include "sector_storage/worker.hpp"

#include <boost/optional.hpp>

namespace fc::sector_storage {
  using primitives::TaskType;

  using WorkerId = uint64_t;

  class Estimator {
   public:
    virtual ~Estimator() = default;

    virtual void startWork(WorkerId, TaskType, CallId) = 0;

    virtual void finishWork(CallId) = 0;

    virtual void abortWork(CallId) = 0;

    /** Returns average time for task type for worker. */
    virtual boost::optional<double> getTime(WorkerId, TaskType) const = 0;
  };

}  // namespace fc::sector_storage
