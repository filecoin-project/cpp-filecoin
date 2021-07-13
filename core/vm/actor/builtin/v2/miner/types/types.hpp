/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/v0/miner/types/types.hpp"

namespace fc::vm::actor::builtin::v2::miner {

  struct MinerInfo : types::miner::MinerInfo {};
  CBOR_TUPLE(MinerInfo,
             owner,
             worker,
             control,
             pending_worker_key,
             peer_id,
             multiaddrs,
             seal_proof_type,
             sector_size,
             window_post_partition_sectors,
             consensus_fault_elapsed,
             pending_owner_address)

  using Deadline = v0::miner::Deadline;

}  // namespace fc::vm::actor::builtin::v2::miner
