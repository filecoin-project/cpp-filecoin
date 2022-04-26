/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/market/pending_proposals.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::v3::market {
  using types::Universal;

  struct PendingProposals : types::market::PendingProposals {
    outcome::result<void> loadRoot() const override;
    outcome::result<bool> has(const Key &key) const override;
    outcome::result<void> set(
        const Key &key,
        const Universal<types::market::DealProposal> &value) override;
    outcome::result<void> remove(const Key &key) override;
  };

  inline CBOR2_DECODE(PendingProposals) {
    return s >> v.pending_proposals_3;
  }

  inline CBOR2_ENCODE(PendingProposals) {
    return s << v.pending_proposals_3;
  }

}  // namespace fc::vm::actor::builtin::v3::market

namespace fc::cbor_blake {
  template <>
  struct CbVisitT<vm::actor::builtin::v3::market::PendingProposals> {
    template <typename Visitor>
    static void call(vm::actor::builtin::v3::market::PendingProposals &p,
                     const Visitor &visit) {
      visit(p.pending_proposals_3);
    }
  };
}  // namespace fc::cbor_blake
