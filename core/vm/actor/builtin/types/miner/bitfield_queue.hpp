/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "adt/stop.hpp"
#include "common/outcome.hpp"
#include "primitives/rle_bitset/rle_bitset.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::ChainEpoch;
  using primitives::RleBitset;

  template <size_t bits>
  struct BitfieldQueue {
    adt::Array<RleBitset, bits> queue;
    QuantSpec quant;

    inline outcome::result<void> addToQueue(ChainEpoch raw_epoch,
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

    inline outcome::result<void> cut(const RleBitset &to_cut) {
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

    inline outcome::result<void> addManyToQueueValues(
        const std::map<ChainEpoch, RleBitset> &values) {
      std::map<ChainEpoch, RleBitset> quantized_values;
      std::vector<ChainEpoch> updated_epochs;

      for (auto const &[raw_epoch, entries] : values) {
        const auto epoch = quant.quantizeUp(raw_epoch);
        updated_epochs.push_back(epoch);
        quantized_values[epoch] += entries;
      }

      std::sort(updated_epochs.begin(),
                updated_epochs.end(),
                std::less<ChainEpoch>());

      for (auto epoch : updated_epochs) {
        OUTCOME_TRY(addToQueue(epoch, quantized_values[epoch]));
      }

      return outcome::success();
    }

    inline outcome::result<std::tuple<RleBitset, bool>> popUntil(
        ChainEpoch until) {
      std::vector<RleBitset> popped_values;
      std::vector<ChainEpoch> popped_keys;

      auto visitor{[&](auto epoch, const auto &buf) -> outcome::result<void> {
        if (static_cast<ChainEpoch>(epoch) > until) {
          return outcome::failure(adt::kStopError);
        }
        popped_keys.push_back(epoch);
        popped_values.push_back(buf);
        return outcome::success();
      }};

      CATCH_STOP(queue.visit(visitor));

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
  };

}  // namespace fc::vm::actor::builtin::types::miner

namespace fc::cbor_blake {
  template <size_t bits>
  struct CbVisitT<vm::actor::builtin::types::miner::BitfieldQueue<bits>> {
    template <typename Visitor>
    static void call(vm::actor::builtin::types::miner::BitfieldQueue<bits> &p,
                     const Visitor &visit) {
      visit(p.queue);
    }
  };
}  // namespace fc::cbor_blake
