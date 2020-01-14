/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP

#include "codec/cbor/cbor.hpp"
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/vm_context.hpp"

namespace fc::vm::actor {
  using common::Buffer;

  /// Actor method signature
  using ActorMethod = std::function<outcome::result<Buffer>(
      const Actor &, VMContext &, gsl::span<const uint8_t>)>;

  /// Actor methods exported by number
  using ActorExports = std::map<uint64_t, ActorMethod>;

  constexpr VMExitCode DECODE_ACTOR_PARAMS_ERROR{1};

  /// Decode actor params, raises appropriate error
  template <typename T>
  outcome::result<T> decodeActorParams(gsl::span<const uint8_t> params_bytes) {
    auto maybe_params = codec::cbor::decode<T>(params_bytes);
    if (!maybe_params) {
      return DECODE_ACTOR_PARAMS_ERROR;
    }
    return maybe_params;
  }
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
