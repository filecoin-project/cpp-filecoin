/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/circulating.hpp"

#include "const.hpp"
#include "primitives/block/block.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm {
  using actor::builtin::states::StateProvider;

  outcome::result<TokenAmount> getLocked(StateTreePtr state_tree) {
    const auto ipld{state_tree->getStore()};
    TokenAmount locked;
    StateProvider provider(ipld);

    OUTCOME_TRY(market_actor, state_tree->get(actor::kStorageMarketAddress));
    OUTCOME_TRY(market_state, provider.getMarketActorState(market_actor));
    locked += market_state->total_client_locked_collateral
              + market_state->total_provider_locked_collateral
              + market_state->total_client_storage_fee;

    OUTCOME_TRY(power_actor, state_tree->get(actor::kStoragePowerAddress));
    OUTCOME_TRY(power_state, provider.getPowerActorState(power_actor));
    locked += power_state->total_pledge_collateral;

    return locked;
  }

  outcome::result<std::shared_ptr<Circulating>> Circulating::make(
      IpldPtr ipld, const CID &genesis) {
    auto circulating{std::make_shared<Circulating>()};
    OUTCOME_TRY(block, ipld->getCbor<primitives::block::BlockHeader>(genesis));
    OUTCOME_TRYA(circulating->genesis,
                 getLocked(std::make_shared<state::StateTreeImpl>(
                     ipld, block.parent_state_root)));
    return circulating;
  }

  outcome::result<TokenAmount> Circulating::circulating(
      StateTreePtr state_tree, ChainEpoch epoch) const {
    const auto ipld{state_tree->getStore()};
    TokenAmount vested;
    TokenAmount mined;

    auto vest{[&](auto days, TokenAmount amount) {
      ChainEpoch duration(days * kEpochsInDay);
      auto elapsed{epoch};
      if (epoch > kUpgradeIgnitionHeight) {
        amount *= kFilecoinPrecision;
        elapsed -= kUpgradeLiftoffHeight;
      }
      if (elapsed >= duration) {
        vested += amount;
      } else if (elapsed >= 0) {
        vested += amount - (duration - elapsed) * bigdiv(amount, duration);
      }
    }};
    constexpr auto sixMonths{183};
    constexpr auto year{365};
    auto calico{epoch > kUpgradeCalicoHeight};
    vest(sixMonths, (calico ? 19015887 : 49929341) + 32787700);
    vest(year, 22421712 + (calico ? 9400000 : 0));
    vest(2 * year, 7223364);
    vest(3 * year, 87637883 + (calico ? 898958 : 0));
    vest(6 * year, 100000000 + 300000000 + (calico ? 9805053 : 0));
    if (calico) {
      vest(0, 10632000);
    }
    if (epoch <= kUpgradeActorsV2Height) {
      vested += genesis;
    }

    OUTCOME_TRY(reward_actor, state_tree->get(actor::kRewardAddress));
    StateProvider provider(ipld);
    OUTCOME_TRY(reward_state, provider.getRewardActorState(reward_actor));
    mined = reward_state->total_reward;

    TokenAmount disbursed;
    if (epoch > kUpgradeActorsV2Height) {
      OUTCOME_TRY(reserve, state_tree->get(actor::kReserveActorAddress));
      disbursed =
          TokenAmount{kFilReserve} * kFilecoinPrecision - reserve.balance;
    }

    OUTCOME_TRY(burn, state_tree->get(actor::kBurntFundsActorAddress));
    OUTCOME_TRY(locked, getLocked(state_tree));
    return vested + mined + disbursed - burn.balance - locked;
  }
}  // namespace fc::vm
