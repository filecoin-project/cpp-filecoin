/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/runtime/circulating.hpp"

#include "const.hpp"
#include "primitives/block/block.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/market/actor.hpp"
#include "vm/actor/builtin/v0/miner/policy.hpp"
#include "vm/actor/builtin/v0/reward/reward_actor.hpp"
#include "vm/actor/builtin/v0/storage_power/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v2/market/actor.hpp"
#include "vm/actor/builtin/v2/power/actor.hpp"
#include "vm/actor/builtin/v2/reward/actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"
#include "vm/version.hpp"

namespace fc::vm {
  outcome::result<TokenAmount> getLocked(StateTreePtr state_tree) {
    auto ipld{state_tree->getStore()};
    TokenAmount locked;
    OUTCOME_TRY(market, state_tree->get(actor::kStorageMarketAddress));
    if (market.code == actor::builtin::v0::kStorageMarketCodeCid
        || market.code == actor::builtin::v2::kStorageMarketCodeCid) {
      static_assert(std::is_same_v<actor::builtin::v0::market::State,
                                   actor::builtin::v2::market::State>);
      OUTCOME_TRY(
          state, ipld->getCbor<actor::builtin::v0::market::State>(market.head));
      locked += state.total_client_locked_collateral
                + state.total_provider_locked_collateral
                + state.total_client_storage_fee;
    } else {
      return std::errc::owner_dead;
    }
    OUTCOME_TRY(power, state_tree->get(actor::kStoragePowerAddress));
    if (power.code == actor::builtin::v0::kStoragePowerCodeCid) {
      OUTCOME_TRY(
          state,
          ipld->getCbor<actor::builtin::v0::storage_power::State>(power.head));
      locked += state.total_pledge;
    } else if (power.code == actor::builtin::v2::kStoragePowerCodeCid) {
      OUTCOME_TRY(state,
                  ipld->getCbor<actor::builtin::v2::power::State>(power.head));
      locked += state.total_pledge;
    } else {
      return std::errc::owner_dead;
    }
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
    auto ipld{state_tree->getStore()};
    TokenAmount vested, mined;

    auto vest{[&](auto days, TokenAmount amount) {
      ChainEpoch duration(days * vm::actor::builtin::v0::miner::kEpochsInDay);
      auto elapsed{epoch};
      if (epoch > version::kUpgradeIgnitionHeight) {
        amount *= kFilecoinPrecision;
        elapsed -= version::kUpgradeLiftoffHeight;
      }
      if (elapsed >= duration) {
        vested += amount;
      } else if (elapsed >= 0) {
        vested += amount - (duration - elapsed) * bigdiv(amount, duration);
      }
    }};
    constexpr auto sixMonths{183}, year{365};
    auto calico{epoch > version::kUpgradeCalicoHeight};
    vest(sixMonths, (calico ? 19015887 : 49929341) + 32787700);
    vest(year, 22421712 + (calico ? 9400000 : 0));
    vest(2 * year, 7223364);
    vest(3 * year, 87637883 + (calico ? 898958 : 0));
    vest(6 * year, 100000000 + 300000000 + (calico ? 9805053 : 0));
    if (calico) {
      vest(0, 10632000);
    }
    if (epoch <= version::kUpgradeActorsV2Height) {
      vested += genesis;
    }

    OUTCOME_TRY(reward, state_tree->get(actor::kRewardAddress));
    if (reward.code == actor::builtin::v0::kRewardActorCodeID) {
      OUTCOME_TRY(
          state, ipld->getCbor<actor::builtin::v0::reward::State>(reward.head));
      mined = state.total_mined;
    } else if (reward.code == actor::builtin::v2::kRewardActorCodeID) {
      OUTCOME_TRY(
          state, ipld->getCbor<actor::builtin::v2::reward::State>(reward.head));
      mined = state.total_storage_power_reward;
    } else {
      return std::errc::owner_dead;
    }

    TokenAmount disbursed;
    if (epoch > version::kUpgradeActorsV2Height) {
      OUTCOME_TRY(reserve, state_tree->get(actor::kReserveActorAddress));
      disbursed =
          TokenAmount{kFilReserve} * kFilecoinPrecision - reserve.balance;
    }

    OUTCOME_TRY(burn, state_tree->get(actor::kBurntFundsActorAddress));
    OUTCOME_TRY(locked, getLocked(state_tree));
    return vested + mined + disbursed - burn.balance - locked;
  }
}  // namespace fc::vm
