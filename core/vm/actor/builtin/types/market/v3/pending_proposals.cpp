/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/market/v3/pending_proposals.hpp"

namespace fc::vm::actor::builtin::v3::market {

  outcome::result<void> PendingProposals::loadRoot() const {
    return pending_proposals_3.hamt.loadRoot();
  }

  outcome::result<bool> PendingProposals::has(const Key &key) const {
    return pending_proposals_3.has(key);
  }

  outcome::result<void> PendingProposals::set(const Key &key,
                                              const DealProposal &value) {
    return pending_proposals_3.set(key);
  }

  outcome::result<void> PendingProposals::remove(const Key &key) {
    return pending_proposals_3.remove(key);
  }

}  // namespace fc::vm::actor::builtin::v3::market
