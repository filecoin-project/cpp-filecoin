/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/init/init_actor.hpp"

#include "vm/actor/builtin/v3/codes.hpp"

namespace fc::vm::actor::builtin::v3::init {
  bool canExec(const CID &caller_code_id, const CID &exec_code_id) {
    bool result = false;
    if (exec_code_id == kStorageMinerCodeId) {
      result = caller_code_id == kStoragePowerCodeId;
    } else if (exec_code_id == kPaymentChannelCodeCid
               || exec_code_id == kMultisigCodeId) {
      result = true;
    }
    return result;
  }

  // Exec
  //============================================================================

  outcome::result<Exec::Result> Exec::execute(Runtime &runtime,
                                              const Exec::Params &params,
                                              CallerAssert caller_assert,
                                              ExecAssert exec_assert) {
    return v2::init::Exec::execute(runtime, params, caller_assert, canExec);
  }

  ACTOR_METHOD_IMPL(Exec) {
    auto caller_assert = [&runtime](bool condition) -> outcome::result<void> {
      return runtime.vm_assert(condition);
    };

    return execute(runtime, params, caller_assert, canExec);
  }

  //============================================================================

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Exec>(),
  };

}  // namespace fc::vm::actor::builtin::v3::init
