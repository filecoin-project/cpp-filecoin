/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "const.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::verified_registry {
  using primitives::StoragePower;

  extern StoragePower kMinVerifiedDealSize;

  void setPolicy(const StoragePower &minVerifiedDealSize);

}  // namespace fc::vm::actor::builtin::types::verified_registry
