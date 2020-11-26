/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/account/account_actor.hpp"

namespace fc::vm::actor::builtin::v2::account {

  const ActorExports exports{
      exportMethod<Construct>(),
      exportMethod<PubkeyAddress>(),
  };
}  // namespace fc::vm::actor::builtin::v2::account
