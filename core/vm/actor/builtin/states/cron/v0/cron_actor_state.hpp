/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/states/cron/cron_actor_state.hpp"

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::v0::cron {
  struct CronActorState : states::CronActorState {};
  CBOR_TUPLE(CronActorState, entries)
}  // namespace fc::vm::actor::builtin::v0::cron
