/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_NODE_API_STORAGE_PROVIDER_NODE_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_NODE_API_STORAGE_PROVIDER_NODE_HPP

#include <functional>
#include <utility>
#include <vector>

#include <gsl/span>
#include <libp2p/connection/stream.hpp>
#include "crypto/signature/signature.hpp"
#include "markets/storage/common.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {
  using TipSetToken = std::vector<uint8_t>;
  using crypto::signature::Signature;
  using libp2p::connection::Stream;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::PieceDescriptor;
  using primitives::TokenAmount;
  using primitives::address::Address;

  using DealSectorCommittedCallback =
      std::function<void(outcome::result<void>)>;

  /// Part of Fuhon node interface to be called by StorageProvider
  class StorageProviderNode {
   public:
    virtual ~StorageProviderNode() = default;

    virtual auto getChainHead()
        -> outcome::result<std::pair<TipSetToken, ChainEpoch>> = 0;

    /// Verify signature validity against an address and data
    virtual auto verifySignature(const Signature &signature,
                                 const Address &signer,
                                 gsl::span<const uint8_t> plaintext)
        -> bool = 0;

    /// Adds funds with the StorageMinerActor for a storage participant
    /// Used by both providers and clients
    virtual auto addFunds(const Address &address, const TokenAmount &amount)
        -> outcome::result<void> = 0;

    /// Ensures that a storage market participant has a certain amount of
    /// available funds. If additional funds are needed, they will be sent from
    /// the 'wallet' address
    virtual auto ensureFunds(const Address &address,
                             const Address &wallet,
                             const TokenAmount &amount)
        -> outcome::result<void> = 0;

    /// GetBalance returns locked/unlocked for a storage participant
    /// Used by both providers and clients
    virtual auto getBalance(const Address &address)
        -> outcome::result<Balance> = 0;

    /// Publishes deal on chain
    virtual auto publishDeals(const MinerDeal &deal)
        -> outcome::result<std::pair<DealId, CID>> = 0;

    /// Lists all deals associated with a storage provider
    virtual auto listProviderDeals(const Address &address)
        -> outcome::result<std::vector<StorageDeal>> = 0;

    /// Produces a signature for the passed data using the key of a signer
    virtual auto signBytes(const Address &signer, gsl::span<uint8_t> data)
        -> outcome::result<std::shared_ptr<Signature>> = 0;

    /// Retrieves piece placement details
    virtual auto locatePieceForDealWithinSector(DealId deal_id)
        -> outcome::result<PieceDescriptor> = 0;

    /// Called when a deal is complete and on chain, and data has been
    /// transferred and is ready to be added to a sector
    virtual auto onDealComplete(const MinerDeal &deal,
                                UnpaddedPieceSize piece_size,
                                Stream &io) -> outcome::result<void> = 0;

    /// Has to be called after a deal has been committed to a sector and
    /// activated
    virtual auto onDealSectorCommitted(const Address &provider,
                                       DealId deal_id,
                                       DealSectorCommittedCallback cb)
        -> outcome::result<void> = 0;
  };
}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_NODE_API_STORAGE_PROVIDER_NODE_HPP
