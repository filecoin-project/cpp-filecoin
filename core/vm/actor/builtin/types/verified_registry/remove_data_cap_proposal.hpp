/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/address/address.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::verified_registry {
  using primitives::DataCap;
  using primitives::address::Address;

  struct RmDcProposalId {
    uint64_t proposal_id{};

    inline bool operator==(const RmDcProposalId &other) const {
      return proposal_id == other.proposal_id;
    }

    inline bool operator!=(const RmDcProposalId &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(RmDcProposalId, proposal_id)

  struct RemoveDataCapProposal {
    Address verified_client;
    DataCap amount;
    RmDcProposalId removal_proposal_id;

    inline bool operator==(const RemoveDataCapProposal &other) const {
      return verified_client == other.verified_client && amount == other.amount
             && removal_proposal_id == other.removal_proposal_id;
    }

    inline bool operator!=(const RemoveDataCapProposal &other) const {
      return !(*this == other);
    }
  };

}  // namespace fc::vm::actor::builtin::types::verified_registry
