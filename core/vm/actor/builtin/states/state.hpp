/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "common/outcome.hpp"

#define ACTOR_STATE_TO_CBOR_THIS(T)                 \
  ::fc::outcome::result<Buffer> T::toCbor() const { \
    return ::fc::cbor_blake::cbEncodeT(*this);      \
  }

namespace fc::vm::actor::builtin::states {
  using common::Buffer;

  struct State {
    State() = default;
    virtual ~State() = default;

    virtual outcome::result<Buffer> toCbor() const = 0;
  };
}  // namespace fc::vm::actor::builtin::states
