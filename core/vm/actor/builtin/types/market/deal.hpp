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

  struct DealProposal {
    inline TokenAmount clientBalanceRequirement() const {
      return client_collateral + getTotalStorageFee();
    }

    inline TokenAmount providerBalanceRequirement() const {
      return provider_collateral;
    }

    inline EpochDuration duration() const {
      return end_epoch - start_epoch;
    }

    inline TokenAmount getTotalStorageFee() const {
      return storage_price_per_epoch * duration();
    }

    inline CID cid() const {
      OUTCOME_EXCEPT(bytes, codec::cbor::encode(*this));
      return {CID::Version::V1,
              CID::Multicodec::DAG_CBOR,
              Hasher::blake2b_256(bytes)};
    }

    CID piece_cid;
    PaddedPieceSize piece_size;
    bool verified = false;
    Address client;
    Address provider;
    std::string label;
    ChainEpoch start_epoch{};
    ChainEpoch end_epoch{};
    TokenAmount storage_price_per_epoch;
    TokenAmount provider_collateral;
    TokenAmount client_collateral;

    inline bool operator==(const DealProposal &other) const {
      return piece_cid == other.piece_cid && piece_size == other.piece_size
             && client == other.client && provider == other.provider
             && start_epoch == other.start_epoch && end_epoch == other.end_epoch
             && storage_price_per_epoch == other.storage_price_per_epoch
             && provider_collateral == other.provider_collateral
             && client_collateral == other.client_collateral;
    }

    inline bool operator!=(const DealProposal &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(DealProposal,
             piece_cid,
             piece_size,
             verified,
             client,
             provider,
             label,
             start_epoch,
             end_epoch,
             storage_price_per_epoch,
             provider_collateral,
             client_collateral)

  struct DealState {
    ChainEpoch sector_start_epoch;
    ChainEpoch last_updated_epoch;
    ChainEpoch slash_epoch;

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
    DealProposal proposal;
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

  struct StorageParticipantBalance {
    TokenAmount locked;
    TokenAmount available;
  };

}  // namespace fc::vm::actor::builtin::types::market
