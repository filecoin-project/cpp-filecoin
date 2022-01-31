/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "proofs/impl/proof_engine_impl.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"

namespace fc::blockchain::block_validator {
  using crypto::randomness::Randomness;
  using primitives::RleBitset;
  using primitives::address::Address;
  using primitives::sector::ExtendedSectorInfo;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::version::NetworkVersion;

  inline outcome::result<std::vector<ExtendedSectorInfo>>
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  getSectorsForWinningPoSt(NetworkVersion network,
                           const Address &miner,
                           const MinerActorStatePtr &state,
                           const Randomness &rand) {
    std::vector<ExtendedSectorInfo> sectors;
    RleBitset sectors_bitset;
    OUTCOME_TRY(deadlines, state->deadlines.get());
    for (const auto &deadline_cid : deadlines.due) {
      OUTCOME_TRY(deadline, deadline_cid.get());
      OUTCOME_TRY(deadline->partitions.visit([&](auto, auto &part) {
        for (const auto &sector : part->sectors) {
          if (network >= NetworkVersion::kVersion7) {
            if (part->terminated.has(sector) || part->unproven.has(sector)) {
              continue;
            }
          }
          if (!part->faults.has(sector)) {
            sectors_bitset.insert(sector);
          }
        }
        return outcome::success();
      }));
    }
    if (!sectors_bitset.empty()) {
      OUTCOME_TRY(miner_info, state->getInfo());
      OUTCOME_TRY(
          win_type,
          getRegisteredWinningPoStProof(miner_info->window_post_proof_type));
      static proofs::ProofEngineImpl proofs;
      OUTCOME_TRY(indices,
                  proofs.generateWinningPoStSectorChallenge(
                      win_type, miner.getId(), rand, sectors_bitset.size()));
      std::vector<uint64_t> sector_ids{sectors_bitset.begin(),
                                       sectors_bitset.end()};
      for (const auto &i : indices) {
        OUTCOME_TRY(sector, state->sectors.sectors.get(sector_ids[i]));
        sectors.push_back({
            sector->seal_proof,
            sector->sector,
            sector->sector_key_cid,
            sector->sealed_cid,
        });
      }
    }
    return sectors;
  }
}  // namespace fc::blockchain::block_validator

namespace fc {
  using blockchain::block_validator::getSectorsForWinningPoSt;
}  // namespace fc
