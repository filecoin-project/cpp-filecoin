/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::payment_channel {
  using primitives::EpochDuration;

  constexpr size_t kLaneLimit{INT64_MAX};
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern EpochDuration kSettleDelay;
  constexpr size_t kMaxSecretSize{256};
}  // namespace fc::vm::actor::builtin::types::payment_channel
