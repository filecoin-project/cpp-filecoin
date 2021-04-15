/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

namespace fc {
  inline std::pair<size_t, size_t> memoryUsage();
}  // namespace fc

#if __APPLE__

#include <mach/mach_init.h>
#include <mach/task.h>
#include <unistd.h>

std::pair<size_t, size_t> fc::memoryUsage() {
  task_t task{MACH_PORT_NULL};
  if (task_for_pid(current_task(), getpid(), &task) == KERN_SUCCESS) {
    task_basic_info info{};
    mach_msg_type_number_t size{TASK_BASIC_INFO_COUNT};
    if (task_info(task, TASK_BASIC_INFO, (task_info_t)&info, &size)
        == KERN_SUCCESS) {
      return {info.virtual_size, info.resident_size};
    }
  }
  return {};
}

#elif __linux__

#include <unistd.h>
#include <fstream>

std::pair<size_t, size_t> fc::memoryUsage() {
  size_t size{}, rss{};
  std::ifstream{"/proc/self/statm"} >> size >> rss;
  static auto page{getpagesize()};
  return {size * page, rss * page};
}

#endif
