/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/utils/init_actor_utils.hpp"

namespace fc::vm::actor::builtin::v3::init {
  using runtime::Runtime;

  class InitUtils : public utils::init::InitUtils {
   public:
    InitUtils(Runtime &r) : utils::init::InitUtils::InitUtils(r) {}

    outcome::result<void> assertCaller(bool condition) const override;
  };
}  // namespace fc::vm::actor::builtin::v3::init
