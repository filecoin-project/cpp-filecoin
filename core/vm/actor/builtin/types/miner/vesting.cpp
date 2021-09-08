/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/vesting.hpp"

#include "common/container_utils.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using common::slice;

  TokenAmount VestingFunds::unlockVestedFunds(ChainEpoch curr_epoch) {
    TokenAmount amount_unlocked{0};

    int64_t last_index_to_remove = -1;
    for (size_t i = 0; i < funds.size(); i++) {
      if (funds[i].epoch >= curr_epoch) {
        break;
      }

      amount_unlocked += funds[i].amount;
      last_index_to_remove = i;
    }

    if (last_index_to_remove != -1) {
      funds = slice(funds, last_index_to_remove + 1);
    }

    return amount_unlocked;
  }

  void VestingFunds::addLockedFunds(ChainEpoch curr_epoch,
                                    const TokenAmount &vesting_sum,
                                    ChainEpoch proving_period_start,
                                    const VestSpec &spec) {
    std::map<ChainEpoch, size_t> epoch_to_index;
    for (size_t i = 0; i < funds.size(); i++) {
      epoch_to_index[funds[i].epoch] = i;
    }

    const auto vest_begin = curr_epoch + spec.initial_delay;
    TokenAmount vested_so_far{0};
    for (auto e = vest_begin + spec.step_duration; vested_so_far < vesting_sum;
         e += spec.step_duration) {
      const QuantSpec quant(spec.quantization, proving_period_start);
      const auto vest_epoch = quant.quantizeUp(e);
      const auto elapsed = vest_epoch - vest_begin;

      const TokenAmount target_vest =
          elapsed < spec.vest_period
              ? bigdiv(vesting_sum * elapsed, spec.vest_period)
              : vesting_sum;

      const TokenAmount vest_this_time = target_vest - vested_so_far;
      vested_so_far = target_vest;

      if (epoch_to_index.find(vest_epoch) != epoch_to_index.end()) {
        const auto index = epoch_to_index[vest_epoch];
        funds[index].amount += vest_this_time;
      } else {
        funds.push_back(Fund{.epoch = vest_epoch, .amount = vest_this_time});
        epoch_to_index[vest_epoch] = funds.size() - 1;
      }
    }

    std::sort(funds.begin(), funds.end(), [](const Fund &lhs, const Fund &rhs) {
      return lhs.epoch < rhs.epoch;
    });
  }

  TokenAmount VestingFunds::unlockUnvestedFunds(ChainEpoch curr_epoch,
                                                const TokenAmount &target) {
    TokenAmount amount_unlocked{0};
    int64_t last_index_to_remove = -1;
    int64_t start_index_to_remove = 0;

    for (size_t i = 0; i < funds.size(); i++) {
      if (amount_unlocked >= target) {
        break;
      }

      auto &vf = funds[i];

      if (vf.epoch >= curr_epoch) {
        const TokenAmount unlock_amount =
            std::min(TokenAmount(target - amount_unlocked), vf.amount);
        amount_unlocked += unlock_amount;
        const auto new_amount = vf.amount - unlock_amount;

        if (new_amount == 0) {
          last_index_to_remove = i;
        } else {
          vf.amount = new_amount;
        }
      } else {
        start_index_to_remove = i + 1;
      }
    }

    if (last_index_to_remove != -1) {
      decltype(funds) new_funds = slice(funds, 0, start_index_to_remove);
      const auto second_part = slice(funds, last_index_to_remove + 1);
      new_funds.insert(new_funds.end(), second_part.begin(), second_part.end());
      funds = new_funds;
    }

    return amount_unlocked;
  }

}  // namespace fc::vm::actor::builtin::types::miner
