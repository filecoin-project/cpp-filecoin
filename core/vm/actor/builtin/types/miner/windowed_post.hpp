/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::sector::PoStProof;

  struct WindowedPoSt {
    /**
     * Partitions proved by this WindowedPoSt.
     */
    RleBitset partitions;

    /**
     * Array of proofs, one per distinct registered proof type present in the
     * sectors being proven. In the usual case of a single proof type, this
     * array will always have a single element (independent of number of
     * partitions).
     */
    std::vector<PoStProof> proofs;
  };
  CBOR_TUPLE(WindowedPoSt, partitions, proofs)

}  // namespace fc::vm::actor::builtin::types::miner
