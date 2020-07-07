/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_REQUEST_QUEUE_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_REQUEST_QUEUE_HPP

#include <vector>
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::sector_storage {

  using fc::primitives::TaskType;
  using fc::primitives::sector::SectorId;

  struct WorkerRequest {
    SectorId sector;
    TaskType task_type;
    int priority;
  };

  inline bool operator<(const WorkerRequest &lhs, const WorkerRequest &rhs) {
    return lhs.priority > rhs.priority || lhs.task_type < rhs.task_type
           || lhs.sector.sector < rhs.sector.sector;
  }

  class RequestQueue {
   public:
    bool insert(const WorkerRequest &request);

    boost::optional<WorkerRequest> deque();

    std::vector<WorkerRequest>::const_iterator cbegin() const;

    std::vector<WorkerRequest>::const_iterator cend() const;

    bool remove(const std::vector<WorkerRequest>::const_iterator &iterator);

    size_t size() const;

   private:
    std::vector<WorkerRequest> queue_;
  };
}  // namespace fc::sector_storage
#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_REQUEST_QUEUE_HPP
