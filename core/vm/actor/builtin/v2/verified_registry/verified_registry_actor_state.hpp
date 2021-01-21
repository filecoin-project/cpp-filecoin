/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/verified_registry/verified_registry_actor_state.hpp"

namespace fc::vm::actor::builtin::v2::verified_registry {
  using primitives::StoragePower;
  using primitives::address::Address;
  using v0::verified_registry::DataCap;

  using v0::verified_registry::kMinVerifiedDealSize;

  using State = v0::verified_registry::State;

}  // namespace fc::vm::actor::builtin::v2::verified_registry
