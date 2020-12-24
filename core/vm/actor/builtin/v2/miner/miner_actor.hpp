/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/miner_actor.hpp"

namespace fc::vm::actor::builtin::v2::miner {

  using Construct = v0::miner::Construct;
  using OnDeferredCronEvent = v0::miner::OnDeferredCronEvent;
  using ConfirmSectorProofsValid = v0::miner::ConfirmSectorProofsValid;

}  // namespace fc::vm::actor::builtin::v2::miner
