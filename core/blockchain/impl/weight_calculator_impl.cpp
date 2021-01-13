/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/power/actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::weight, WeightCalculatorError, e) {
  using E = fc::blockchain::weight::WeightCalculatorError;
  switch (e) {
    case E::kNoNetworkPower:
      return "No network power";
  }
  return "WeightCalculatorError: unknown error";
}

namespace fc::blockchain::weight {
  using primitives::BigInt;
  using vm::actor::kStoragePowerAddress;
  using vm::state::StateTreeImpl;
  using vm::state::StateTreeVersion;

  using StoragePowerActorState_v0 =
      vm::actor::builtin::v0::storage_power::StoragePowerActorState;
  using StoragePowerActorState_v2 = vm::actor::builtin::v2::power::State;

  constexpr uint64_t kWRatioNum{1};
  constexpr uint64_t kWRatioDen{2};
  constexpr uint64_t kBlocksPerEpoch{5};

  WeightCalculatorImpl::WeightCalculatorImpl(std::shared_ptr<Ipld> ipld)
      : ipld_{std::move(ipld)} {}

  outcome::result<BigInt> WeightCalculatorImpl::calculateWeight(
      const Tipset &tipset) {
    auto state_tree = StateTreeImpl{ipld_, tipset.getParentStateRoot()};

    BigInt network_power;

    if (state_tree.version() > StateTreeVersion::kVersion0) {
      OUTCOME_TRY(
          state,
          state_tree.state<StoragePowerActorState_v2>(kStoragePowerAddress));
      network_power = state.total_qa_power;
    } else {
      OUTCOME_TRY(
          state,
          state_tree.state<StoragePowerActorState_v0>(kStoragePowerAddress));
      network_power = state.total_qa_power;
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
