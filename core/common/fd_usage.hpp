/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdlib>

namespace fc {
  /// Returns the number of fds opened by the current process.
  inline size_t fdUsage();
}  // namespace fc

#if __APPLE__

#include <libproc.h>
#include <unistd.h>
#include <vector>

size_t fc::fdUsage() {
  auto r{proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, nullptr, 0)};
  if (r < 0) {
    return 0;
  }
  std::vector<proc_fdinfo> list;
  list.resize(r / sizeof(proc_fdinfo));
  r = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, list.data(), r);
  if (r < 0) {
    return 0;
  }
  return r / sizeof(proc_fdinfo);
}

#elif __linux__

#include <filesystem>

size_t fc::fdUsage() {
  return std::distance(std::filesystem::directory_iterator{"/proc/self/fd"},
                       {});
}

#endif
