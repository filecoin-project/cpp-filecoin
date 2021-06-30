/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"

namespace fc::vm::actor::builtin::types::miner {

  outcome::result<void> BitfieldQueue::addToQueue(ChainEpoch raw_epoch,
                                                  const RleBitset &values) {
    if (values.empty()) {
      // nothing to do
      return outcome::success();
    }

    const auto epoch = quant.quantizeUp(raw_epoch);
    RleBitset bitfield;

    OUTCOME_TRY(maybe_bitfield, queue.tryGet(epoch));
    if (maybe_bitfield) {
      bitfield = maybe_bitfield.value();
    }

    bitfield += values;
    OUTCOME_TRY(queue.set(epoch, bitfield));
    return outcome::success();
  }

  outcome::result<void> BitfieldQueue::cut(const RleBitset &to_cut) {
    std::vector<ChainEpoch> epochs_to_remove;

    auto visitor{[&](auto epoch, auto &buf) -> outcome::result<void> {
      const auto buffer = buf.cut(to_cut);
      if (!buffer.empty()) {
        OUTCOME_TRY(queue.set(epoch, buffer));
        return outcome::success();
      }
      epochs_to_remove.push_back(epoch);
      return outcome::success();
    }};

    OUTCOME_TRY(queue.visit(visitor));

    for (const auto &epoch : epochs_to_remove) {
      OUTCOME_TRY(queue.remove(epoch));
    }
    return outcome::success();
  }

  outcome::result<void> BitfieldQueue::addManyToQueueValues(
      const std::map<ChainEpoch, std::vector<uint64_t>> &values) {
    std::map<ChainEpoch, std::vector<uint64_t>> quantized_values;
    std::vector<ChainEpoch> updated_epochs;

    for (auto const &[raw_epoch, entries] : values) {
      const auto epoch = quant.quantizeUp(raw_epoch);
      updated_epochs.push_back(epoch);
      quantized_values[epoch].insert(
          quantized_values[epoch].end(), entries.begin(), entries.end());
    }

    std::sort(
        updated_epochs.begin(), updated_epochs.end(), std::less<ChainEpoch>());

    for (auto epoch : updated_epochs) {
      OUTCOME_TRY(addToQueue(epoch,
                             RleBitset(quantized_values[epoch].begin(),
                                       quantized_values[epoch].end())));
    }

    return outcome::success();
  }

  outcome::result<std::tuple<RleBitset, bool>> BitfieldQueue::popUntil(
      ChainEpoch until) {
    std::vector<RleBitset> popped_values;
    std::vector<ChainEpoch> popped_keys;

    auto visitor{[&](auto epoch, const auto &buf) -> outcome::result<void> {
      if (static_cast<ChainEpoch>(epoch) > until) {
        return outcome::success();
      }
      popped_keys.push_back(epoch);
      popped_values.push_back(buf);
      return outcome::success();
    }};

    OUTCOME_TRY(queue.visit(visitor));

    if (popped_keys.empty()) {
      return std::make_tuple(RleBitset{}, false);
    }

    for (const auto &key : popped_keys) {
      OUTCOME_TRY(queue.remove(key));
    }

    RleBitset merged;
    for (const auto &value : popped_values) {
      merged += value;
    }

    return std::make_tuple(merged, true);
  }

}  // namespace fc::vm::actor::builtin::types::miner
