/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/circulating.hpp"

#include "cbor_blake/ipld_version.hpp"
#include "const.hpp"
#include "primitives/block/block.hpp"
#include "vm/actor/builtin/states/market/market_actor_state.hpp"
#include "vm/actor/builtin/states/reward/reward_actor_state.hpp"
#include "vm/actor/builtin/states/storage_power/storage_power_actor_state.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::vm {
  using actor::builtin::states::MarketActorStatePtr;
  using actor::builtin::states::PowerActorStatePtr;
  using actor::builtin::states::RewardActorStatePtr;

  outcome::result<TokenAmount> getLocked(StateTreePtr state_tree) {
    const auto ipld{state_tree->getStore()};
    TokenAmount locked;

    OUTCOME_TRY(market_actor, state_tree->get(actor::kStorageMarketAddress));
    OUTCOME_TRY(market_state,
                getCbor<MarketActorStatePtr>(ipld, market_actor.head));
    locked += market_state->total_client_locked_collateral
              + market_state->total_provider_locked_collateral
              + market_state->total_client_storage_fee;

    OUTCOME_TRY(power_actor, state_tree->get(actor::kStoragePowerAddress));
    OUTCOME_TRY(power_state,
                getCbor<PowerActorStatePtr>(ipld, power_actor.head));
    locked += power_state->total_pledge_collateral;

    return locked;
  }

  outcome::result<std::shared_ptr<Circulating>> Circulating::make(
      IpldPtr ipld, const CID &genesis) {
    ipld = withVersion(ipld, 0);
    auto circulating{std::make_shared<Circulating>()};
    OUTCOME_TRY(block, getCbor<primitives::block::BlockHeader>(ipld, genesis));
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
    OUTCOME_TRY(reward_state,
                getCbor<RewardActorStatePtr>(ipld, reward_actor.head));
    mined = reward_state->total_reward;

    TokenAmount disbursed;
    if (epoch > kUpgradeActorsV2Height) {
      OUTCOME_TRY(reserve, state_tree->get(actor::kReserveActorAddress));
      disbursed =
          TokenAmount{kFilReserve} * kFilecoinPrecision - reserve.balance;
    }

    OUTCOME_TRY(burn, state_tree->get(actor::kBurntFundsActorAddress));
    OUTCOME_TRY(locked, getLocked(state_tree));
    return std::max<TokenAmount>(
        0, vested + mined + disbursed - burn.balance - locked);
  }
}  // namespace fc::vm
