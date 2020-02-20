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

  /// Reserved method number for send operation
  constexpr MethodNumber kSendMethodNumber{0};

  /// Reserved method number for constructor
  constexpr MethodNumber kConstructorMethodNumber{1};

  /// Decode actor params, raises appropriate error
  template <typename T>
  outcome::result<T> decodeActorParams(MethodParams params_bytes) {
    auto maybe_params = codec::cbor::decode<T>(params_bytes);
    if (!maybe_params) {
      return VMExitCode::DECODE_ACTOR_PARAMS_ERROR;
    }
    return maybe_params;
  }

  /// Encode actor params, raises appropriate error
  template <typename T>
  outcome::result<MethodParams> encodeActorParams(const T &params) {
    auto maybe_bytes = codec::cbor::encode(params);
    if (!maybe_bytes) {
      return VMExitCode::ENCODE_ACTOR_PARAMS_ERROR;
    }
    return MethodParams{maybe_bytes.value()};
  }

  template <typename T>
  outcome::result<T> decodeActorReturn(const InvocationOutput &result) {
    OUTCOME_TRY(decoded, codec::cbor::decode<T>(result.return_value.toVector()));
    return decoded;
  }

  template <typename T>
  outcome::result<InvocationOutput> encodeActorReturn(const T &result) {
    OUTCOME_TRY(encoded, codec::cbor::encode(result));
    return InvocationOutput{Buffer{encoded}};
  }

}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
