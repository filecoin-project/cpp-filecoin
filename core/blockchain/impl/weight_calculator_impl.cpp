/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

#include "cbor_blake/ipld_version.hpp"
#include "common/logger.hpp"
#include "vm/actor/builtin/states/power_actor_state.hpp"
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
  using primitives::StoragePower;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::states::PowerActorStatePtr;
  using vm::state::StateTreeImpl;

  constexpr uint64_t kWRatioNum{1};
  constexpr uint64_t kWRatioDen{2};
  constexpr uint64_t kBlocksPerEpoch{5};

  WeightCalculatorImpl::WeightCalculatorImpl(std::shared_ptr<Ipld> ipld)
      : ipld_{std::move(ipld)} {}

  outcome::result<BigInt> WeightCalculatorImpl::calculateWeight(
      const Tipset &tipset) {
    const auto ipld{withVersion(ipld_, tipset.height())};
    OUTCOME_TRY(actor,
                StateTreeImpl{ipld, tipset.getParentStateRoot()}.get(
                    kStoragePowerAddress));
    OUTCOME_TRY(state, getCbor<PowerActorStatePtr>(ipld, actor.head));
    const StoragePower network_power = state->total_qa_power;

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
