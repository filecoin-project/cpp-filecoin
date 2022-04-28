/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/hasher/hasher.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/types/market/deal_proposal.hpp"
#include "vm/actor/builtin/types/universal/universal.hpp"

namespace fc::vm::actor::builtin::types::market {
  using crypto::Hasher;
  using crypto::signature::Signature;
  using libp2p::multi::HashType;
  using primitives::ChainEpoch;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::cid::kCommitmentBytesLen;
  using primitives::piece::PaddedPieceSize;

  enum class BalanceLockingReason : int {
    kClientCollateral,
    kClientStorageFee,
    kProviderCollateral
  };

  inline const CidPrefix kPieceCIDPrefix{
      .version = static_cast<uint64_t>(CID::Version::V1),
      .codec =
          static_cast<uint64_t>(CID::Multicodec::FILECOIN_COMMITMENT_UNSEALED),
      .mh_type = HashType::sha2_256_trunc254_padded,
      .mh_length = kCommitmentBytesLen};

  struct DealState {
    ChainEpoch sector_start_epoch{};
    ChainEpoch last_updated_epoch{};
    ChainEpoch slash_epoch{};

    inline bool operator==(const DealState &other) const {
      return sector_start_epoch == other.sector_start_epoch
             && last_updated_epoch == other.last_updated_epoch
             && slash_epoch == other.slash_epoch;
    }

    inline bool operator!=(const DealState &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(DealState, sector_start_epoch, last_updated_epoch, slash_epoch)

  struct ClientDealProposal {
    Universal<DealProposal> proposal;
    Signature client_signature;

    inline CID cid() const {
      OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
      return {
          CID::Version::V1, CID::Multicodec::DAG_CBOR, Hasher::sha2_256(bytes)};
    }

    inline bool operator==(const ClientDealProposal &other) const {
      return proposal == other.proposal
             && client_signature == other.client_signature;
    }

    inline bool operator!=(const ClientDealProposal &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(ClientDealProposal, proposal, client_signature)

}  // namespace fc::vm::actor::builtin::types::market
