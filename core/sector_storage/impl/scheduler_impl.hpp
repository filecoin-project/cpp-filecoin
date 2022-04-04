/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/scheduler.hpp"

#include <boost/asio/io_context.hpp>
#include <future>
#include <mutex>
#include <unordered_map>
#include <utility>
#include "common/map_utils.hpp"
#include "primitives/resources/resources.hpp"
#include "storage/buffer_map.hpp"

namespace fc::sector_storage {
  using storage::BufferMap;
  using WorkerID = uint64_t;
  using primitives::Resources;

  struct TaskRequest {
    inline TaskRequest(const SectorRef &sector,
                       TaskType task_type,
                       uint64_t priority,
                       std::shared_ptr<WorkerSelector> sel,
                       WorkerAction prepare,
                       WorkerAction work,
                       ReturnCb cb)
        : sector(sector),
          task_type(std::move(task_type)),
          priority(priority),
          sel(std::move(sel)),
          prepare(std::move(prepare)),
          work(std::move(work)),
          cb(std::move(cb)) {
      const auto &resource_table{primitives::getResourceTable()};
      const auto resource_iter = resource_table.find(this->task_type);
      if (resource_iter != resource_table.end()) {
        need_resources = resource_iter->second.at(sector.proof_type);
      }
    };

    SectorRef sector;
    TaskType task_type;
    uint64_t priority;
    std::shared_ptr<WorkerSelector> sel;

    Resources need_resources;

    WorkerAction prepare;
    WorkerAction work;

    ReturnCb cb;
  };

  inline bool operator<(const TaskRequest &lhs, const TaskRequest &rhs) {
    // Get TaskType order if present. For TaskTypes not present order is 0.
    static std::map<TaskType, int> order = {
        {primitives::kTTFinalize, -2},
        {primitives::kTTFetch, -1},
        {primitives::kTTReadUnsealed, 0},
        {primitives::kTTUnseal, 1},
        {primitives::kTTCommit1, 2},
        {primitives::kTTCommit2, 3},
        {primitives::kTTPreCommit2, 4},
        {primitives::kTTPreCommit1, 5},
        {primitives::kTTProveReplicaUpdate1, 6},
        {primitives::kTTProveReplicaUpdate2, 7},
        {primitives::kTTReplicaUpdate, 8},
        {primitives::kTTAddPiece, 9},
        {primitives::kTTRegenSectorKey, 10}};
    const auto left_order = common::getOrDefault(order, lhs.task_type, 0);
    const auto right_order = common::getOrDefault(order, rhs.task_type, 0);

    return less(lhs.priority,
                rhs.priority,
                left_order,
                right_order,
                lhs.sector.id.sector,
                rhs.sector.id.sector);
  }

  class SchedulerImpl : public Scheduler {
   public:
    static outcome::result<std::shared_ptr<SchedulerImpl>> newScheduler(
        std::shared_ptr<boost::asio::io_context> io_context,
        std::shared_ptr<BufferMap> datastore);

    outcome::result<void> schedule(
        const SectorRef &sector,
        const primitives::TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        const ReturnCb &cb,
        uint64_t priority,
        const boost::optional<WorkId> &maybe_work_id) override;

    void newWorker(std::unique_ptr<WorkerHandle> worker) override;

    outcome::result<void> returnResult(const CallId &call_id,
                                       CallResult result) override;

   private:
    explicit SchedulerImpl(std::shared_ptr<boost::asio::io_context> io_context,
                           std::shared_ptr<BufferMap> datastore);

    outcome::result<void> resetWorks();

    outcome::result<bool> maybeScheduleRequest(
        const std::shared_ptr<TaskRequest> &request);

    void assignWorker(WorkerID wid,
                      const std::shared_ptr<WorkerHandle> &worker,
                      const std::shared_ptr<TaskRequest> &request);

    void freeWorker(WorkerID wid);

    std::mutex workers_lock_;
    WorkerID current_worker_id_;
    std::unordered_map<WorkerID, std::shared_ptr<WorkerHandle>> workers_;

    std::mutex cbs_lock_;
    std::map<CallId, ReturnCb> callbacks_;
    std::map<CallId, CallResult> results_;

    std::shared_ptr<BufferMap> call_kv_;

    std::mutex request_lock_;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::multiset<std::shared_ptr<TaskRequest>,
                  std::owner_less<std::shared_ptr<TaskRequest>>>
        request_queue_;

    std::shared_ptr<boost::asio::io_context> io_;

    common::Logger logger_;

    std::atomic<size_t> active_jobs{};
  };

}  // namespace fc::sector_storage
