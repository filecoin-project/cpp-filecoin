/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/request_queue.hpp"

namespace fc::sector_storage {

  bool RequestQueue::insert(const WorkerRequest &request) {
    if (queue_.size() == queue_.max_size()) {
      return false;
    }

    queue_.push_back(request);

    maxHeapify(queue_.size() - 1);

    return true;
  }

  boost::optional<WorkerRequest> RequestQueue::pop() {
    if (queue_.empty()) {
      return boost::none;
    }
    auto res = queue_[0];
    queue_[0] = queue_[queue_.size() - 1];

    minHeapify(0);

    queue_.pop_back();
    return std::move(res);
  }

  boost::optional<WorkerRequest> RequestQueue::at(int index) const {
    if (0 > index || static_cast<uint>(index) < queue_.size()) {
      return boost::none;
    }

    return queue_.at(index);
  }

  size_t RequestQueue::size() const {
    return queue_.size();
  }

  bool RequestQueue::remove(int index) {
    if (0 > index) {
      return false;
    }

    auto current_index = queue_.size() - 1;
    if (current_index != static_cast<uint>(index)) {
      queue_[index] = queue_[current_index];
      if (!minHeapify(index)) {
        maxHeapify(index);
      }
    }

    queue_.pop_back();
    return true;
  }

  bool RequestQueue::minHeapify(int index) {
    if (0 > index) {
      return false;
    }
    auto request = queue_[index];
    int current_index = index;
    int smallest;
    int right_child_index;
    while (current_index <= ((std::numeric_limits<int>::max() - 1) / 2)
           && static_cast<uint>(2 * current_index + 1) < queue_.size()) {
      smallest = 2 * current_index + 1;
      if (current_index <= ((std::numeric_limits<int>::max() - 2) / 2)
          && static_cast<uint>(2 * current_index + 2) < queue_.size()) {
        right_child_index = 2 * current_index + 2;
        if (queue_[right_child_index] < queue_[smallest]) {
          smallest = right_child_index;
        }
      }
      queue_[current_index] = queue_[smallest];
      current_index = smallest;
    }
    if (index != current_index) {
      queue_[current_index] = request;
      return true;
    }
    return false;
  }

  void RequestQueue::maxHeapify(int index) {
    if (0 > index) {
      return;
    }
    auto request = queue_[index];
    int current_index = index;
    int parent_index = (current_index - 1) / 2;
    while (current_index != parent_index && request < queue_[parent_index]) {
      queue_[current_index] = queue_[parent_index];
      current_index = parent_index;
      parent_index = (current_index - 1) / 2;
    }
    if (current_index != index) {
      queue_[current_index] = request;
    }
  }
}  // namespace fc::sector_storage
