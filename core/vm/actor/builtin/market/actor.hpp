/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP

#include "adt/array.hpp"
#include "adt/balance_table.hpp"
#include "adt/empty_value.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin::market {
  using adt::AddressKeyer;
  using adt::BalanceTable;
  using adt::EmptyValue;
  using adt::UvarintKeyer;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::DealWeight;
  using primitives::EpochDuration;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using primitives::piece::PaddedPieceSize;
  using primitives::sector::RegisteredProof;
  using Ipld = storage::ipfs::IpfsDatastore;

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

  inline bool operator==(const DealProposal &lhs, const DealProposal &rhs) {
    return lhs.piece_cid == rhs.piece_cid && lhs.piece_size == rhs.piece_size
           && lhs.client == rhs.client && lhs.provider == rhs.provider
           && lhs.start_epoch == rhs.start_epoch
           && lhs.end_epoch == rhs.end_epoch
           && lhs.storage_price_per_epoch == rhs.storage_price_per_epoch
           && lhs.provider_collateral == rhs.provider_collateral
           && lhs.client_collateral == rhs.client_collateral;
  }

  struct DealState {
    ChainEpoch sector_start_epoch;
    ChainEpoch last_updated_epoch;
    ChainEpoch slash_epoch;
  };
  CBOR_TUPLE(DealState, sector_start_epoch, last_updated_epoch, slash_epoch)

  struct State {
    using PartyDeals = adt::Map<EmptyValue, UvarintKeyer>;

    void load(std::shared_ptr<Ipld> ipld);
    outcome::result<void> flush();
    outcome::result<DealState> getState(DealId deal_id);
    outcome::result<void> addDeal(DealId deal_id, const DealProposal &deal);

    adt::Array<DealProposal> proposals;
    adt::Array<DealState> states;
    BalanceTable escrow_table;
    BalanceTable locked_table;
    DealId next_deal;
    adt::Map<PartyDeals, AddressKeyer> deals_by_party;
  };
  CBOR_TUPLE(State,
             proposals,
             states,
             escrow_table,
             locked_table,
             next_deal,
             deals_by_party)

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

  TokenAmount clientPayment(ChainEpoch epoch,
                            const DealProposal &deal,
                            const DealState &deal_state);

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

  extern ActorExports exports;
}  // namespace fc::vm::actor::builtin::market

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
