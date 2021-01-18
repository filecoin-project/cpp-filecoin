/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "node/fwd.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::vm::actor::cgo {
  using message::UnsignedMessage;
  using primitives::StoragePower;
  using primitives::sector::RegisteredSealProof;
  using runtime::Execution;

  void config(const StoragePower &min_verified_deal_size,
              const StoragePower &consensus_miner_min_power,
              const std::vector<RegisteredSealProof> &supported_proofs);

  void configMainnet();

  outcome::result<Buffer> invoke(const std::shared_ptr<Execution> &exec,
                                 const UnsignedMessage &message,
                                 const CID &code,
                                 size_t method,
                                 BytesIn params);
}  // namespace fc::vm::actor::cgo
