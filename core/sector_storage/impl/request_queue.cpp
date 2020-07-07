/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/request_queue.hpp"

namespace fc::sector_storage {

  bool RequestQueue::insert(const WorkerRequest &request) {
    size_t max_index = queue_.size();
    if (max_index == queue_.max_size()) {
      return false;
    }

    if (max_index == 0) {
      queue_.push_back(request);
      return true;
    }

    size_t min_index = 0;
    size_t current_index = (min_index + max_index) / 2;
    while (min_index < max_index && current_index != 0) {
      if (request < queue_[current_index]) {
        max_index = current_index - 1;
      } else {
        min_index = current_index + 1;
      }
      current_index = (min_index + max_index) / 2;
    }

    if (request < queue_[min_index]) {
      queue_.insert(queue_.begin() + min_index, request);
    } else if (min_index + 1 > queue_.size()) {
      queue_.push_back(request);
    } else {
      queue_.insert(queue_.begin() + min_index + 1, request);
    }

    return true;
  }

  size_t RequestQueue::size() const {
    return queue_.size();
  }

  bool RequestQueue::remove(
      const std::vector<WorkerRequest>::const_iterator &iterator) {
    queue_.erase(iterator);
    return true;
  }

  boost::optional<WorkerRequest> RequestQueue::deque() {
    if (queue_.empty()) {
      return boost::none;
    }

    auto res = *queue_.cbegin();
    queue_.erase(queue_.cbegin());
    return res;
  }

  std::vector<WorkerRequest>::const_iterator RequestQueue::cbegin() const {
    return queue_.cbegin();
  }

  std::vector<WorkerRequest>::const_iterator RequestQueue::cend() const {
    return queue_.cend();
  }
}  // namespace fc::sector_storage
