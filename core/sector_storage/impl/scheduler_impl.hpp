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

namespace fc::sector_storage {
  using WorkerID = uint64_t;

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
          cb(std::move(cb)){};

    SectorRef sector;
    TaskType task_type;
    uint64_t priority;
    std::shared_ptr<WorkerSelector> sel;

    WorkerAction prepare;
    WorkerAction work;

    ReturnCb cb;
  };

  inline bool operator<(const TaskRequest &lhs, const TaskRequest &rhs) {
    return less(rhs.priority,
                lhs.priority,
                lhs.task_type,
                rhs.task_type,
                lhs.sector.id.sector,
                rhs.sector.id.sector);
  }

  class SchedulerImpl : public Scheduler {
   public:
    explicit SchedulerImpl(std::shared_ptr<boost::asio::io_context> io_context);

    outcome::result<void> schedule(
        const SectorRef &sector,
        const primitives::TaskType &task_type,
        const std::shared_ptr<WorkerSelector> &selector,
        const WorkerAction &prepare,
        const WorkerAction &work,
        const ReturnCb &cb,
        uint64_t priority) override;

    void newWorker(std::unique_ptr<WorkerHandle> worker) override;

    outcome::result<void> returnAddPiece(
        const CallId &call_id,
        const PieceInfo &maybe_piece_info,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnSealPreCommit1(
        const CallId &call_id,
        const PreCommit1Output &maybe_precommit1_out,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnSealPreCommit2(
        const CallId &call_id,
        const SectorCids &maybe_sector_cids,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnSealCommit1(
        const CallId &call_id,
        const Commit1Output &maybe_commit1_out,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnSealCommit2(
        const CallId &call_id,
        const Proof &maybe_proof,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnFinalizeSector(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnMoveStorage(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnUnsealPiece(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnReadPiece(
        const CallId &call_id,
        bool maybe_status,
        const boost::optional<CallError> &maybe_error) override;

    outcome::result<void> returnFetch(
        const CallId &call_id,
        const boost::optional<CallError> &maybe_error) override;

   private:
    outcome::result<void> returnResult(const CallId &call_id,
                                       CallResult result);

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

    std::mutex request_lock_;
    std::multiset<std::shared_ptr<TaskRequest>,
                  std::owner_less<std::shared_ptr<TaskRequest>>>
        request_queue_;

    std::shared_ptr<boost::asio::io_context> io_;

    common::Logger logger_;

    std::atomic<size_t> active_jobs{};
  };

}  // namespace fc::sector_storage
