/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
#define CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP

#include "adt/array.hpp"
#include "adt/balance_table.hpp"
#include "adt/set.hpp"
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

    CID cid() const;

    CID piece_cid;
    PaddedPieceSize piece_size;
    bool verified;
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
             verified,
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
    using DealSet = adt::Set<UvarintKeyer>;

    adt::Array<DealProposal> proposals;
    adt::Array<DealState> states;
    BalanceTable escrow_table;
    BalanceTable locked_table;
    DealId next_deal;
    adt::Map<DealSet, UvarintKeyer> deals_by_epoch;
    ChainEpoch last_cron;
  };
  CBOR_TUPLE(State,
             proposals,
             states,
             escrow_table,
             locked_table,
             next_deal,
             deals_by_epoch,
             last_cron)

  struct ClientDealProposal {
    DealProposal proposal;
    Signature client_signature;

    CID cid() const;

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

  struct PublishStorageDeals : ActorMethodBase<4> {
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

  struct VerifyDealsOnSectorProveCommit : ActorMethodBase<5> {
    struct Params {
      std::vector<DealId> deals;
      ChainEpoch sector_expiry;
    };
    struct Result {
      DealWeight deal_weight, verified_deal_weight;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(VerifyDealsOnSectorProveCommit::Params, deals, sector_expiry)
  CBOR_TUPLE(VerifyDealsOnSectorProveCommit::Result,
             deal_weight,
             verified_deal_weight)

  struct OnMinerSectorsTerminate : ActorMethodBase<6> {
    struct Params {
      std::vector<DealId> deals;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(OnMinerSectorsTerminate::Params, deals)

  struct ComputeDataCommitment : ActorMethodBase<7> {
    struct Params {
      std::vector<DealId> deals;
      RegisteredProof sector_type;
    };
    using Result = CID;
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ComputeDataCommitment::Params, deals, sector_type)

  struct CronTick : ActorMethodBase<8> {
    ACTOR_METHOD_DECL();
  };

  extern const ActorExports exports;
}  // namespace fc::vm::actor::builtin::market

namespace fc {
  template <>
  struct Ipld::Visit<vm::actor::builtin::market::State> {
    template <typename F>
    static void f(vm::actor::builtin::market::State &s, const F &f) {
      f(s.proposals);
      f(s.states);
      f(s.escrow_table);
      f(s.locked_table);
      f(s.deals_by_epoch);
    }
  };
}  // namespace fc

#endif  // CPP_FILECOIN_CORE_VM_ACTOR_BUILTIN_MARKET_ACTOR_HPP
