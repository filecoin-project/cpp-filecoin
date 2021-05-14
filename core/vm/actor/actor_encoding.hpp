/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "common/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::vm::actor {
  using runtime::InvocationOutput;

  /// Type for empty params and result
  struct None {};
  CBOR_ENCODE(None, none) {
    return s;
  }
  CBOR_DECODE(None, none) {
    return s;
  }

  /// Decode actor params, raises appropriate error
  template <typename T>
  outcome::result<T> decodeActorParams(MethodParams params_bytes, bool v7) {
    if constexpr (std::is_same_v<T, None>) {
      return T{};
    }
    auto maybe_params = codec::cbor::decode<T>(params_bytes);
    if (!maybe_params) {
      return outcome::failure(v7 ? VMExitCode::kErrSerialization
                                 : VMExitCode::kOldErrActorFailure);
    }
    return maybe_params;
  }

  /// Encode actor params, raises appropriate error
  template <typename T>
  outcome::result<MethodParams> encodeActorParams(const T &params) {
    auto maybe_bytes = codec::cbor::encode(params);
    if (!maybe_bytes) {
      return VMExitCode::kErrSerialization;
    }
    return MethodParams{maybe_bytes.value()};
  }

  template <typename T>
  outcome::result<T> decodeActorReturn(const InvocationOutput &result) {
    if constexpr (std::is_same_v<T, None>) {
      return T{};
    }
    return codec::cbor::decode<T>(result);
  }

  template <typename T>
  outcome::result<InvocationOutput> encodeActorReturn(const T &result) {
    auto maybe_encoded = codec::cbor::encode(result);
    if (!maybe_encoded) {
      return VMExitCode::kEncodeActorResultError;
    }
    return std::move(maybe_encoded.value());
  }
}  // namespace fc::vm::actor
