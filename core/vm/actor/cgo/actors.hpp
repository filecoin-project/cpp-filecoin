/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/bytes.hpp"
#include "common/outcome.hpp"
#include "fwd.hpp"

namespace fc::vm::actor::cgo {
  using runtime::Runtime;

  /**
   * Sets actors parameters with ones defined in profile:
   *  - miner minimal power
   */
  void configParams();

  enum class LogLevel {
    kDebug = -1,
    kInfo,
    kWarn,
    kError,
  };
  void logLevel(LogLevel level);

  outcome::result<Bytes> invoke(const CID &code,
                                const std::shared_ptr<Runtime> &runtime);
}  // namespace fc::vm::actor::cgo
