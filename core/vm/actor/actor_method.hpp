/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime.hpp"

#define ACTOR_METHOD(name)                \
  outcome::result<InvocationOutput> name( \
      const Actor &actor, Runtime &runtime, const MethodParams &params)

namespace fc::vm::actor {

  using common::Buffer;
  using runtime::InvocationOutput;
  using runtime::Runtime;

  /**
   * Actor method signature
   * @param Actor - actor itself
   * @param Runtime - VM context exposed to actors during method execution
   * @param MethodParams - parameters for method call
   * @return InvocationOutput - invocation method result or error occurred
   */
  using ActorMethod = std::function<outcome::result<InvocationOutput>(
      const Actor &, Runtime &, const MethodParams &)>;

  /// Actor methods exported by number
  using ActorExports = std::map<MethodNumber, ActorMethod>;

  /// Decode actor params, raises appropriate error
  template <typename T>
  outcome::result<T> decodeActorParams(MethodParams params_bytes) {
    auto maybe_params = codec::cbor::decode<T>(params_bytes);
    if (!maybe_params) {
      return VMExitCode::DECODE_ACTOR_PARAMS_ERROR;
    }
    return maybe_params;
  }

  using runtime::encodeActorParams;

  template <typename T>
  outcome::result<T> decodeActorReturn(const InvocationOutput &result) {
    OUTCOME_TRY(decoded,
                codec::cbor::decode<T>(result.return_value.toVector()));
    return std::move(decoded);
  }

  template <typename T>
  outcome::result<InvocationOutput> encodeActorReturn(const T &result) {
    OUTCOME_TRY(encoded, codec::cbor::encode(result));
    return InvocationOutput{Buffer{encoded}};
  }

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
