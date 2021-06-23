/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>

#include "cbor_blake/ipld.hpp"
#include "primitives/tipset/chain.hpp"

namespace fc::primitives::tipset::chain::file {
  constexpr auto kRevert{(uint8_t)-1};

  struct Updater {
    std::ofstream hash, count;

    operator bool() const;
    Updater &apply(gsl::span<const CbCid> ts);
    Updater &revert();
    Updater &flush();
  };

  bool loadOrCreate(TsChain &chain,
                    Updater *updater,
                    bool *updated,
                    const std::string &path,
                    const CbIpldPtr &ipld,
                    const std::vector<CbCid> &head_tsk,
                    size_t update_when);
  TsBranchPtr loadOrCreate(bool *updated,
                           const std::string &path,
                           const CbIpldPtr &ipld,
                           const std::vector<CbCid> &head_tsk,
                           size_t update_when);
}  // namespace fc::primitives::tipset::chain::file
