/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/system/system_actor.hpp"

namespace fc::vm::actor::builtin::v3::system {

  const ActorExports exports{exportMethod<Construct>()};

}  // namespace fc::vm::actor::builtin::v3::system
