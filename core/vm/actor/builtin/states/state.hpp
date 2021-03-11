/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "common/outcome.hpp"

namespace fc::vm::actor::builtin::states {
  using common::Buffer;

  struct State {
    State() = default;
    virtual ~State() = default;

    virtual outcome::result<Buffer> toCbor() const = 0;
  };
}  // namespace fc::vm::actor::builtin::states
