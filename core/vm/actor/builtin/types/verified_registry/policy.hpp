/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::verified_registry {
  using primitives::StoragePower;

  inline const StoragePower kMinVerifiedDealSize{1 << 20};

}  // namespace fc::vm::actor::builtin::types::verified_registry
