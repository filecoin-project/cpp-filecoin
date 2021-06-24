/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>
#include <mutex>

#include "cbor_blake/ipld.hpp"
#include "primitives/tipset/chain.hpp"

namespace fc::primitives::tipset::chain::file {
  constexpr auto kRevert{(uint8_t)-1};
  using Seed = BytesN<32>;

  struct Updater {
    std::ofstream file_hash, file_count;
    std::ifstream file_hash_read;
    Bytes counts;
    uint32_t count_sum{};
    std::mutex read_mutex;

    operator bool() const;
    Updater &apply(gsl::span<const CbCid> ts);
    Updater &revert();
    Updater &flush();
  };

  TsBranchPtr loadOrCreate(bool *updated,
                           const std::string &path,
                           const CbIpldPtr &ipld,
                           const std::vector<CbCid> &head_tsk,
                           size_t update_when,
                           size_t lazy_limit);
}  // namespace fc::primitives::tipset::chain::file
