/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

#define CATCH_STOP(expr) OUTCOME_TRY(adt::catchStop((expr)))

namespace fc::adt {

  /**
   * The specific error used to stop adt container's visitor.
   * The same as "break" in "for" loop.
   *
   * NOTE: If your visitor emits this error, you must to catch it.
   * Don't use regular OUTCOME_TRY, use CATCH_STOP
   */
  constexpr auto kStopError = std::errc::interrupted;

  inline outcome::result<void> catchStop(const outcome::result<void> &res) {
    if (res.has_error()) {
      if (res.error() != kStopError) {
        return res.error();
      }
    }
    return outcome::success();
  }

}  // namespace fc::adt
