/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "vm/actor/builtin/states/cron_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::cron {
  struct CronActorState : states::CronActorState {
    CronActorState() : states::CronActorState(ActorVersion::kVersion0) {}
  };
  CBOR_TUPLE(CronActorState, entries)
}  // namespace fc::vm::actor::builtin::v0::cron
