/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/scheduler_impl.hpp"

libp2p::outcome::result<void> fc::sector_storage::SchedulerImpl::schedule(
    const fc::primitives::sector::SectorId &sector,
    const fc::primitives::TaskType &task_type,
    const std::shared_ptr<WorkerSelector> &selector,
    const fc::sector_storage::WorkerAction &prepare,
    const fc::sector_storage::WorkerAction &work,
    uint64_t priority) {
  return fc::outcome::success();
}

libp2p::outcome::result<void> fc::sector_storage::SchedulerImpl::newWorker(
    std::unique_ptr<WorkerHandle> &&worker) {
  return fc::outcome::success();
}

libp2p::outcome::result<void>
fc::sector_storage::SchedulerImpl::maybeScheduleRequest(
    const fc::sector_storage::TaskRequest &request) {
  return fc::outcome::success();
}

libp2p::outcome::result<void> fc::sector_storage::SchedulerImpl::assignWorker(
    const std::pair<WorkerID, std::shared_ptr<WorkerHandle>> &worker,
    const fc::sector_storage::TaskRequest &request) {
  return fc::outcome::success();
}

libp2p::outcome::result<void> fc::sector_storage::SchedulerImpl::freeWorker(
    fc::sector_storage::WorkerID wid) {
  return fc::outcome::success();
}
