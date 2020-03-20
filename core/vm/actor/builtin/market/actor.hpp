/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::market {
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector::RegisteredProof;

  struct DealProposal {
    inline TokenAmount clientBalanceRequirement() const {
      return client_collateral + storage_price_per_epoch * duration();
    }

    inline TokenAmount providerBalanceRequirement() const {
      return provider_collateral;
    }

    inline EpochDuration duration() const {
      return end_epoch - start_epoch;
    }

    CID piece_cid;
    PaddedPieceSize piece_size;
    Address client;
    Address provider;
    ChainEpoch start_epoch;
    ChainEpoch end_epoch;
    TokenAmount storage_price_per_epoch;
    TokenAmount provider_collateral;
    TokenAmount client_collateral;
  };
  CBOR_TUPLE(DealProposal,
             piece_cid,
             piece_size,
             client,
             provider,
             start_epoch,
             end_epoch,
             storage_price_per_epoch,
             provider_collateral,
             client_collateral)

  struct DealState {
    ChainEpoch sector_start_epoch;
    ChainEpoch last_updated_epoch;
    ChainEpoch slash_epoch;
  };
  CBOR_TUPLE(DealState, sector_start_epoch, last_updated_epoch, slash_epoch)

  struct ClientDealProposal {
    DealProposal proposal;
    Signature client_signature;
  };
  CBOR_TUPLE(ClientDealProposal, proposal, client_signature)

  struct OnChainDeal {
    Buffer piece_ref;
    uint64_t piece_size;
    Address client;
    Address provider;
    ChainEpoch proposal_expiration;
    EpochDuration duration;
    TokenAmount storage_price_per_epoch;
    TokenAmount storage_collateral;
    ChainEpoch activation_epoch;
  };

  struct StorageParticipantBalance {
    TokenAmount locked;
    TokenAmount available;
  };

  struct Construct : ActorMethodBase<1> {
    ACTOR_METHOD_DECL();
  };

  struct AddBalance : ActorMethodBase<2> {
    using Params = Address;
    ACTOR_METHOD_DECL();
  };

  struct WithdrawBalance : ActorMethodBase<3> {
    struct Params {
      Address address;
      TokenAmount amount;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(WithdrawBalance::Params, address, amount)

  struct HandleExpiredDeals : ActorMethodBase<4> {
    struct Params {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(HandleExpiredDeals::Params, deals)

  struct PublishStorageDeals : ActorMethodBase<5> {
    struct Params {
      std::vector<ClientDealProposal> deals;
    };
    struct Result {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(PublishStorageDeals::Params, deals)
  CBOR_TUPLE(PublishStorageDeals::Result, deals)

  struct VerifyDealsOnSectorProveCommit : ActorMethodBase<6> {
    struct Params {
      std::vector<DealId> deals;
      ChainEpoch sector_expiry;
    };
    using Result = DealWeight;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsOnSectorProveCommit::Params, deals, sector_expiry)

  struct OnMinerSectorsTerminate : ActorMethodBase<7> {
    struct Params {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnMinerSectorsTerminate::Params, deals)

  struct ComputeDataCommitment : ActorMethodBase<8> {
    struct Params {
      std::vector<DealId> deals;
      RegisteredProof sector_type;
    };
    using Result = CID;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ComputeDataCommitment::Params, deals, sector_type)
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
