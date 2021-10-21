/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/cid_key.hpp"
#include "adt/map.hpp"
#include "adt/set.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::vm::actor::builtin::types::market {
  using adt::CidKeyer;

  struct PendingProposals {
    using Key = CidKeyer::Key;

    adt::Map<DealProposal, CidKeyer> pending_proposals_0;
    adt::Set<CidKeyer> pending_proposals_3;

    virtual ~PendingProposals() = default;

    virtual outcome::result<void> loadRoot() const = 0;
    virtual outcome::result<bool> has(const Key &key) const = 0;
    virtual outcome::result<void> set(const Key &key,
                                      const DealProposal &value) = 0;
    virtual outcome::result<void> remove(const Key &key) = 0;
  };

}  // namespace fc::vm::actor::builtin::types::market
