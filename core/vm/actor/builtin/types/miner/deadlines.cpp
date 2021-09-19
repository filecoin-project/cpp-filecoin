/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/deadlines.hpp"

#include "common/error_text.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using types::miner::kWPoStPeriodDeadlines;

  static const auto kDeadlineIdError = ERROR_TEXT("invalid deadline id");

  outcome::result<Universal<Deadline>> Deadlines::loadDeadline(
      uint64_t deadline_id) const {
    if (deadline_id >= due.size()) {
      return kDeadlineIdError;
    }

    return due[deadline_id].get();
  }

  outcome::result<void> Deadlines::updateDeadline(
      uint64_t deadline_id, const Universal<Deadline> &deadline) {
    if (deadline_id >= due.size()) {
      return kDeadlineIdError;
    }

    OUTCOME_TRY(deadline->validateState());
    OUTCOME_TRY(due[deadline_id].set(deadline));
    return outcome::success();
  }

  outcome::result<std::tuple<uint64_t, uint64_t>> Deadlines::findSector(
      SectorNumber sector_num) const {
    uint64_t dl_id = 0;
    uint64_t part_id = 0;
    bool found = false;

    for (; dl_id < due.size(); dl_id++) {
      OUTCOME_TRY(deadline, due[dl_id].get());

      auto visitor{
          [&](int64_t id, const auto &partition) -> outcome::result<void> {
            if (found) {
              return outcome::success();
            }

            found = partition->sectors.has(sector_num);
            if (found) {
              part_id = id;
            }

            return outcome::success();
          }};

      OUTCOME_TRY(deadline->partitions.visit(visitor));

      if (found) {
        break;
      }
    }

    if (!found) {
      return ERROR_TEXT("sector not due at any deadline");
    }

    return std::make_tuple(dl_id, part_id);
  }

  outcome::result<Deadlines> makeEmptyDeadlines(const IpldPtr &ipld,
                                                const CID &empty_amt_cid) {
    OUTCOME_TRY(deadline, makeEmptyDeadline(ipld, empty_amt_cid));
    OUTCOME_TRY(deadline_cid, setCbor(ipld, deadline));
    adt::CbCidT<Universal<Deadline>> deadline_cid_t{deadline_cid};
    return Deadlines{std::vector(kWPoStPeriodDeadlines, deadline_cid_t)};
  }

}  // namespace fc::vm::actor::builtin::types::miner
