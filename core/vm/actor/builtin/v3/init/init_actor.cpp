/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/init/init_actor.hpp"

namespace fc::vm::actor::builtin::v3::init {

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<Exec>(),
  };
}  // namespace fc::vm::actor::builtin::v3::init
