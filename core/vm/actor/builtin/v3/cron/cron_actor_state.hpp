/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/builtin/states/cron_actor_state.hpp"

namespace fc::vm::actor::builtin::v3::cron {
  struct CronActorState : states::CronActorState {
    CronActorState() : states::CronActorState(ActorVersion::kVersion3) {}
  };
  CBOR_TUPLE(CronActorState, entries)
}  // namespace fc::vm::actor::builtin::v3::cron
