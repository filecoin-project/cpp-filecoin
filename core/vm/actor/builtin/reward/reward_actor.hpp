/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_REWARD_ACTOR_HPP
#define CPP_FILECOIN_REWARD_ACTOR_HPP

#include <boost/optional.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/enum.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"
#include "vm/actor/actor.hpp"

namespace fc::vm::actor::builtin::reward {

  using TokenAmount = primitives::BigInt;

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

}  // namespace fc::vm::actor::builtin::reward

#endif  // CPP_FILECOIN_REWARD_ACTOR_HPP
