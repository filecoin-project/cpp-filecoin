/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP

#include "filecoin/codec/cbor/cbor.hpp"
#include "filecoin/common/outcome.hpp"
#include "filecoin/vm/actor/actor.hpp"
#include "filecoin/vm/actor/actor_encoding.hpp"
#include "filecoin/vm/exit_code/exit_code.hpp"
#include "filecoin/vm/runtime/runtime.hpp"

/// Declare actor method function
#define ACTOR_METHOD_DECL() \
  static outcome::result<Result> call(Runtime &, const Params &);

/// Define actor method function
#define ACTOR_METHOD_IMPL(M) \
  outcome::result<M::Result> M::call(Runtime &runtime, const Params &params)

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
      Runtime &, const MethodParams &)>;

  /// Actor methods exported by number
  using ActorExports = std::map<MethodNumber, ActorMethod>;

  /// Actor method base class
  template <uint64_t number>
  struct ActorMethodBase {
    using Params = None;
    using Result = None;
    static constexpr MethodNumber Number{number};
  };

  /// Generate export table entry
  template <typename M>
  auto exportMethod() {
    return std::make_pair(
        M::Number,
        ActorMethod{[](auto &runtime,
                       auto &params) -> outcome::result<InvocationOutput> {
          OUTCOME_TRY(params2, decodeActorParams<typename M::Params>(params));
          OUTCOME_TRY(result, M::call(runtime, params2));
          return encodeActorReturn(result);
        }});
  }
}  // namespace fc::vm::actor

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_ACTOR_METHOD_HPP
