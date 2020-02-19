/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REWARD_ACTOR_HPP
#define CPP_FILECOIN_REWARD_ACTOR_HPP

#include <boost/optional.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/enum.hpp"
#include "power/power_table.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::reward {

  using TokenAmount = primitives::BigInt;
  using Power = power::Power;

  enum class VestingFunction : uint64_t {
    NONE = 0,
    LINEAR,
  };

  struct Reward {
    VestingFunction vesting_function;
    primitives::ChainEpoch start_epoch;
    primitives::ChainEpoch end_epoch;
    TokenAmount value;
    TokenAmount amount_withdrawn;

    primitives::BigInt amountVested(
        const primitives::ChainEpoch &current_epoch);
  };

  CBOR_ENCODE(Reward, reward) {
    return s << (s.list() << common::to_int(reward.vesting_function)
                          << reward.start_epoch.convert_to<uint64_t>()
                          << reward.end_epoch.convert_to<uint64_t>()
                          << reward.value << reward.amount_withdrawn);
  }

  CBOR_DECODE(Reward, reward) {
    uint64_t vesting_function_value, start_epoch, end_epoch;
    s.list() >> vesting_function_value >> start_epoch >> end_epoch
        >> reward.value >> reward.amount_withdrawn;
    reward.vesting_function =
        static_cast<VestingFunction>(vesting_function_value);
    reward.start_epoch = start_epoch;
    reward.end_epoch = end_epoch;
    return s;
  }

  struct State {
    TokenAmount reward_total;
    CID reward_map;

    outcome::result<void> addReward(
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
        const Address &owner,
        const Reward &reward);

    outcome::result<TokenAmount> withdrawReward(
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &store,
        const Address &owner,
        const primitives::ChainEpoch &current_epoch);
  };

  CBOR_ENCODE(State, state) {
    return s << (s.list() << state.reward_map << state.reward_total);
  }

  CBOR_DECODE(State, state) {
    s.list() >> state.reward_map >> state.reward_total;
    return s;
  }

  // Actor related stuff

  static const BigInt kTokenPrecision{1e18};
  static const BigInt kBlockRewardTarget{1e20};

  static constexpr auto kRewardVestingFunction{VestingFunction::NONE};
  static const primitives::EpochDuration kRewardVestingPeriod{0};

  struct AwardBlockRewardParams {
    Address miner;
    TokenAmount penalty;
    TokenAmount gas_reward;
    Power nominal_power;
  };

  CBOR_ENCODE(AwardBlockRewardParams, v) {
    return s << (s.list() << v.miner << v.penalty << v.gas_reward
                          << v.nominal_power.convert_to<BigInt>());
  }

  CBOR_DECODE(AwardBlockRewardParams, v) {
    BigInt power;
    s.list() >> v.miner >> v.penalty >> v.gas_reward >> power;
    v.nominal_power = power.convert_to<decltype(v.nominal_power)>();
    return s;
  }

  constexpr MethodNumber kAwardBlockRewardMethodNumber{2};
  constexpr MethodNumber kWithdrawRewardMethodNumber{3};

  extern const ActorExports exports;

  class RewardActor {
   public:
    static outcome::result<InvocationOutput> construct(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> awardBlockReward(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

    static outcome::result<InvocationOutput> withdrawReward(
        const Actor &actor, Runtime &runtime, const MethodParams &params);

   private:
    static TokenAmount computeBlockReward(const State &state,
                                          const TokenAmount &balance);
  };

}  // namespace fc::vm::actor::builtin::reward

#endif  // CPP_FILECOIN_REWARD_ACTOR_HPP
