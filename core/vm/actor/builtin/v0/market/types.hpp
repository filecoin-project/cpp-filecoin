/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "crypto/hasher/hasher.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/market_actor_state.hpp"

namespace fc::vm::actor::builtin::v0::market {
  using crypto::Hasher;
  using crypto::signature::Signature;
  using primitives::TokenAmount;
  using states::market::DealProposal;

  enum class BalanceLockingReason : int {
    kClientCollateral,
    kClientStorageFee,
    kProviderCollateral
  };

  struct ClientDealProposal {
    DealProposal proposal;
    Signature client_signature;

    inline CID cid() const {
      OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
      return {
          CID::Version::V1, CID::Multicodec::DAG_CBOR, Hasher::sha2_256(bytes)};
    }

    inline bool operator==(const ClientDealProposal &rhs) const {
      return proposal == rhs.proposal
             && client_signature == rhs.client_signature;
    }
  };
  CBOR_TUPLE(ClientDealProposal, proposal, client_signature)

  struct StorageParticipantBalance {
    TokenAmount locked;
    TokenAmount available;
  };
}  // namespace fc::vm::actor::builtin::v0::market
