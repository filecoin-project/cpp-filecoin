/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/builtin/types/miner/deadline.hpp"
#include "vm/actor/builtin/types/miner/miner_info.hpp"

namespace fc::vm::actor::builtin::v0::miner {

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
             window_post_partition_sectors)

  struct Deadline : types::miner::Deadline {
    Deadline() = default;
    Deadline(const types::miner::Deadline &other)
        : types::miner::Deadline(other) {}
    Deadline(types::miner::Deadline &&other) : types::miner::Deadline(other) {}
  };
  CBOR_TUPLE(Deadline,
             partitions,
             expirations_epochs,
             post_submissions,
             early_terminations,
             live_sectors,
             total_sectors,
             faulty_power)

}  // namespace fc::vm::actor::builtin::v0::miner

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::v0::miner::Deadline> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v0::miner::Deadline &d,
                     const Visitor &visit) {
      visit(d.partitions);
      visit(d.expirations_epochs);
    }
  };
}  // namespace fc
