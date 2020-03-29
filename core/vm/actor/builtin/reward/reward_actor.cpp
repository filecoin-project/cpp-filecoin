/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/reward/reward_actor.hpp"

#include "adt/multimap.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/exit_code/exit_code.hpp"

using fc::adt::Multimap;

namespace fc::vm::actor::builtin::reward {

  TokenAmount computeBlockReward(const State &state,
                                 const TokenAmount &balance);

  const ActorExports exports = {
      exportMethod<AwardBlockReward>(),
      exportMethod<WithdrawReward>(),
  };

  primitives::BigInt Reward::amountVested(
      const primitives::ChainEpoch &current_epoch) {
    auto elapsed = current_epoch - start_epoch;
    switch (vesting_function) {
      case VestingFunction::NONE:
        return value;
      case VestingFunction::LINEAR: {
        auto vest_duration{end_epoch - start_epoch};
        if (elapsed >= vest_duration) {
          return value;
        }
        return (value * elapsed) / vest_duration;
      }
      default:
        return 0;
    }
  }

  outcome::result<void> State::addReward(
      const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
      const Address &owner,
      const Reward &reward) {
    auto rewards = Multimap(store, reward_map);
    auto key_bytes = primitives::address::encode(owner);
    std::string key(key_bytes.begin(), key_bytes.end());
    OUTCOME_TRY(rewards.addCbor(key, reward));
    OUTCOME_TRY(mmap_root, rewards.flush());
    reward_map = mmap_root;
    reward_total += reward.value;
    return outcome::success();
  }

  outcome::result<TokenAmount> State::withdrawReward(
      const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
      const Address &owner,
      const primitives::ChainEpoch &current_epoch) {
    auto rewards = Multimap(store, reward_map);
    auto key_bytes = primitives::address::encode(owner);
    std::string key(key_bytes.begin(), key_bytes.end());

    std::vector<Reward> remaining_rewards;
    TokenAmount withdrawable_sum{0};
    OUTCOME_TRY(rewards.visit(
        key,
        [&current_epoch, &withdrawable_sum, &remaining_rewards](
            auto &&value) -> outcome::result<void> {
          OUTCOME_TRY(reward, codec::cbor::decode<Reward>(value));
          auto unlocked = reward.amountVested(current_epoch);
          auto withdrawable = unlocked - reward.amount_withdrawn;
          if (withdrawable < 0) {
            /*
             * could be good to log somehow these values:
             * - withdrawable
             * - reward
             * - current epoch
             */
            return vm::VMExitCode::REWARD_ACTOR_NEGATIVE_WITHDRAWABLE;
          }
          withdrawable_sum += withdrawable;
          if (unlocked < reward.value) {
            Reward new_reward(reward);
            new_reward.amount_withdrawn = unlocked;
            remaining_rewards.emplace_back(std::move(new_reward));
          }
          return outcome::success();
        }));

    assert(withdrawable_sum < reward_total);

    OUTCOME_TRY(rewards.removeAll(key));
    for (const Reward &rr : remaining_rewards) {
      OUTCOME_TRY(rewards.addCbor(key, rr));
    }
    OUTCOME_TRY(rewards_root, rewards.flush());
    reward_map = rewards_root;
    reward_total -= withdrawable_sum;
    return withdrawable_sum;
  }

  ACTOR_METHOD_IMPL(Construct) {
    if (runtime.getImmediateCaller() != kSystemActorAddress) {
      return VMExitCode::MULTISIG_ACTOR_WRONG_CALLER;
    }

    Multimap empty_mmap{runtime};
    OUTCOME_TRY(empty_mmap_cid, empty_mmap.flush());
    State empty_state{.reward_total = 0, .reward_map = empty_mmap_cid};

    OUTCOME_TRY(runtime.commitState(empty_state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(AwardBlockReward) {
    if (runtime.getImmediateCaller() != kSystemActorAddress) {
      return VMExitCode::REWARD_ACTOR_WRONG_CALLER;
    }
    assert(params.gas_reward == runtime.getValueReceived());
    OUTCOME_TRY(prior_balance, runtime.getCurrentBalance());
    TokenAmount penalty{0};
    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());

    auto block_reward = computeBlockReward(state, params.gas_reward);
    TokenAmount total_reward = block_reward + params.gas_reward;

    penalty = std::min(params.penalty, total_reward);
    auto reward_payable = total_reward - penalty;

    assert(total_reward <= prior_balance);

    if (reward_payable > TokenAmount{0}) {
      auto current_epoch = runtime.getCurrentEpoch();
      Reward new_reward{.vesting_function = kRewardVestingFunction,
                        .start_epoch = current_epoch,
                        .end_epoch = current_epoch + kRewardVestingPeriod,
                        .value = reward_payable,
                        .amount_withdrawn = 0};
      OUTCOME_TRY(state.addReward(runtime, params.miner, new_reward));
    }
    OUTCOME_TRY(runtime.sendFunds(kBurntFundsActorAddress, penalty));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  ACTOR_METHOD_IMPL(WithdrawReward) {
    OUTCOME_TRY(code, runtime.getActorCodeID(runtime.getImmediateCaller()));
    if (!isSignableActor(code)) {
      return VMExitCode::REWARD_ACTOR_WRONG_CALLER;
    }
    auto owner = runtime.getMessage().get().from;

    OUTCOME_TRY(state, runtime.getCurrentActorStateCbor<State>());
    OUTCOME_TRY(
        withdrawn,
        state.withdrawReward(runtime, owner, runtime.getCurrentEpoch()));
    OUTCOME_TRY(runtime.sendFunds(owner, withdrawn));
    OUTCOME_TRY(runtime.commitState(state));
    return outcome::success();
  }

  TokenAmount computeBlockReward(const State &state,
                                 const TokenAmount &balance) {
    TokenAmount treasury = balance - state.reward_total;
    auto target_reward = kBlockRewardTarget;
    return std::min(target_reward, treasury);
  }

}  // namespace fc::vm::actor::builtin::reward
