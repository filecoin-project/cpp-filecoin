/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"

namespace fc::vm::actor::builtin::types::miner {

  enum class CronEventType {
    kWorkerKeyChange,
    kProvingDeadline,
    kProcessEarlyTerminations,
  };

  struct CronEventPayload {
    CronEventType event_type;
  };
  CBOR_TUPLE(CronEventPayload, event_type)

}  // namespace fc::vm::actor::builtin::types::miner
