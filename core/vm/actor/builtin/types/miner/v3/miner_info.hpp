/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/miner/miner_info.hpp"

#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::vm::actor::builtin::v3::miner {

  struct MinerInfo : types::miner::MinerInfo {};
  CBOR_TUPLE(MinerInfo,
             owner,
             worker,
             control,
             pending_worker_key,
             peer_id,
             multiaddrs,
             window_post_proof_type,
             sector_size,
             window_post_partition_sectors,
             consensus_fault_elapsed,
             pending_owner_address)

}  // namespace fc::vm::actor::builtin::v3::miner
