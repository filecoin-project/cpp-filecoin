/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

#include "common/logger.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v2/power/actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::weight, WeightCalculatorError, e) {
  using E = fc::blockchain::weight::WeightCalculatorError;
  switch (e) {
    case E::kNoNetworkPower:
      return "No network power";
  }
}

namespace fc::blockchain::weight {
  using primitives::BigInt;
  using vm::actor::kStoragePowerAddress;
  using vm::state::StateTreeImpl;

  constexpr uint64_t kWRatioNum{1};
  constexpr uint64_t kWRatioDen{2};
  constexpr uint64_t kBlocksPerEpoch{5};

  WeightCalculatorImpl::WeightCalculatorImpl(std::shared_ptr<Ipld> ipld)
      : ipld_{std::move(ipld)} {}

  outcome::result<BigInt> WeightCalculatorImpl::calculateWeight(
      const Tipset &tipset) {
    OUTCOME_TRY(actor,
                StateTreeImpl{ipld_, tipset.getParentStateRoot()}.get(
                    kStoragePowerAddress));
    BigInt network_power;
    if (actor.code == vm::actor::builtin::v0::kStoragePowerCodeCid) {
      OUTCOME_TRY(state,
                  ipld_->getCbor<vm::actor::builtin::v0::storage_power::State>(
                      actor.head));
      network_power = state.total_qa_power;
    } else if (actor.code == vm::actor::builtin::v2::kStoragePowerCodeCid) {
      OUTCOME_TRY(
          state,
          ipld_->getCbor<vm::actor::builtin::v2::power::State>(actor.head));
      network_power = state.total_qa_power;
    } else {
      spdlog::error(
          "WeightCalculatorImpl: unknown power actor code {} at {} {}",
          actor.code,
          tipset.height(),
          fmt::join(tipset.key.cids(), " "));
      return std::errc::owner_dead;
    }
    if (network_power <= 0) {
      return outcome::failure(WeightCalculatorError::kNoNetworkPower);
    }
    BigInt log{boost::multiprecision::msb(network_power) << 8};
    auto j{0};
    for (auto &block : tipset.blks) {
      j += block.election_proof.win_count;
    }
    return tipset.getParentWeight() + log
           + bigdiv(log * j * kWRatioNum, kBlocksPerEpoch * kWRatioDen);
  }

}  // namespace fc::blockchain::weight
