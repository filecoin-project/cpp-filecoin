/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/array.hpp"
#include "adt/balance_table.hpp"
#include "adt/set.hpp"
#include "adt/uvarint_key.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "crypto/hasher/hasher.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"
#include "vm/actor/builtin/states/state.hpp"

namespace fc::vm::actor::builtin::states {
  using adt::BalanceTable;
  using adt::UvarintKeyer;
  using crypto::Hasher;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::EpochDuration;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  namespace market {
    struct CidKeyer {
      using Key = CID;
      static std::string encode(const Key &key) {
        OUTCOME_EXCEPT(bytes, key.toBytes());
        return {bytes.begin(), bytes.end()};
      }
      static outcome::result<Key> decode(const std::string &key) {
        return CID::fromBytes(common::span::cbytes(key));
      }
    };

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
      bool verified;
      Address client;
      Address provider;
      std::string label;
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
               label,
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
  }  // namespace market

  struct MarketActorState : State {
    explicit MarketActorState(ActorVersion version)
        : State(ActorType::kMarket, version) {}

    using DealSet = adt::Set<UvarintKeyer>;

    adt::Array<market::DealProposal> proposals;
    adt::Array<market::DealState> states;
    adt::Map<market::DealProposal, market::CidKeyer> pending_proposals;
    BalanceTable escrow_table;
    BalanceTable locked_table;
    DealId next_deal{0};
    adt::Map<DealSet, UvarintKeyer> deals_by_epoch;
    ChainEpoch last_cron{-1};
    TokenAmount total_client_locked_collateral{};
    TokenAmount total_provider_locked_collateral{};
    TokenAmount total_client_storage_fee{};
  };

  using MarketActorStatePtr = std::shared_ptr<MarketActorState>;
}  // namespace fc::vm::actor::builtin::states
