/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/fwd.hpp"
#include "primitives/sector/sector.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::cgo {
  using message::UnsignedMessage;
  using primitives::StoragePower;
  using primitives::sector::RegisteredProof;
  using runtime::Runtime;

  void config(const StoragePower &min_verified_deal_size,
              const StoragePower &consensus_miner_min_power,
              const std::vector<RegisteredProof> &supported_proofs);

  outcome::result<Buffer> invoke(const CID &code,
                                 const std::shared_ptr<Runtime> &runtime);
}  // namespace fc::vm::actor::cgo
