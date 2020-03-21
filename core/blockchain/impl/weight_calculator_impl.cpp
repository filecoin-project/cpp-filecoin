/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/weight_calculator_impl.hpp"

#include "vm/actor/builtin/storage_power/storage_power_actor_state.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

/* LCOV_EXCL_START */
OUTCOME_CPP_DEFINE_CATEGORY(fc::blockchain::weight, WeightCalculatorError, e) {
  using E = fc::blockchain::weight::WeightCalculatorError;
  switch (e) {
    case E::NO_NETWORK_POWER:
      return "No network power";
  }
}
/* LCOV_EXCL_STOP */

namespace fc::blockchain::weight {
  using primitives::BigInt;
  using vm::actor::kStoragePowerAddress;
  using vm::actor::builtin::storage_power::StoragePowerActorState;
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
    OUTCOME_TRY(state, ipld_->getCbor<StoragePowerActorState>(actor.head));
    auto network_power = state.total_network_power;
    if (network_power <= 0) {
      return outcome::failure(WeightCalculatorError::NO_NETWORK_POWER);
    }
    BigInt log{boost::multiprecision::msb(network_power) << 8};
    return tipset.getParentWeight() + log
           + (log * tipset.blks.size() * kWRatioNum)
                 / (kBlocksPerEpoch * kWRatioDen);
  }

}  // namespace fc::blockchain::weight
