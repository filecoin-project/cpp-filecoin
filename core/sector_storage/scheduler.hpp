/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"
#include "sector_storage/selector.hpp"
#include "sector_storage/worker.hpp"

namespace fc::sector_storage {
  using primitives::TaskType;
  using primitives::sector::SectorId;

  struct WorkId {
    TaskType task_type;
    libp2p::common::Hash256 param_hash = {};

    explicit operator Bytes() const {
      Bytes result;
      append(result, common::span::cbytes(task_type));
      result.push_back('(');
      append(result, param_hash);
      result.push_back(')');
      return result;
    }
  };
  inline bool operator==(const WorkId &lhs, const WorkId &rhs) {
    return lhs.task_type == rhs.task_type and lhs.param_hash == rhs.param_hash;
  }

  CBOR_TUPLE(WorkId, task_type, param_hash);

  enum class WorkStatus : uint64_t {
    kUndefined = 0,
    kStart,
    kInProgress,
  };

  struct WorkState {
    WorkId id;

    WorkStatus status = WorkStatus::kUndefined;

    CallId call_id;
  };
  CBOR_TUPLE(WorkState, id, status, call_id);

  using WorkerAction =
      std::function<outcome::result<CallId>(const std::shared_ptr<Worker> &)>;

  constexpr uint64_t kDefaultTaskPriority = 0;

  class Scheduler {
   public:
    virtual ~Scheduler() = default;

    virtual outcome::result<void> schedule(
        const SectorRef &sector,
        const TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        const ReturnCb &cb,
        uint64_t priority,
        const boost::optional<WorkId> &maybe_work_id) = 0;

    virtual void newWorker(std::unique_ptr<WorkerHandle> worker) = 0;

    virtual outcome::result<void> returnResult(const CallId &call_id,
                                               CallResult result) = 0;
  };

  enum class SchedulerErrors {
    kCannotSelectWorker = 0,
    kNotFoundWorker,
  };
}  // namespace fc::sector_storage

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage, SchedulerErrors);
