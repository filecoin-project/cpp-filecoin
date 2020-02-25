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
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::reward {

  using power::Power;
  using primitives::TokenAmount;

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

  CBOR_TUPLE(
      Reward, vesting_function, start_epoch, end_epoch, value, amount_withdrawn)

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

  CBOR_TUPLE(State, reward_map, reward_total)

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

  CBOR_TUPLE(AwardBlockRewardParams, miner, penalty, gas_reward, nominal_power)

  constexpr MethodNumber kAwardBlockRewardMethodNumber{2};
  constexpr MethodNumber kWithdrawRewardMethodNumber{3};

  extern const ActorExports exports;

  class RewardActor {
   public:
    static ACTOR_METHOD(construct);

    static ACTOR_METHOD(awardBlockReward);

    static ACTOR_METHOD(withdrawReward);

   private:
    static TokenAmount computeBlockReward(const State &state,
                                          const TokenAmount &balance);
  };

}  // namespace fc::vm::actor::builtin::reward

#endif  // CPP_FILECOIN_REWARD_ACTOR_HPP
