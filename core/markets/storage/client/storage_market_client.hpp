/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/client/client_deal.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "markets/storage/status_protocol.hpp"
#include "markets/storage/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"
#include "storage/filestore/filestore.hpp"

namespace fc::markets::storage::client {
  using fc::storage::filestore::FileStore;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::RegisteredSealProof;

  class StorageMarketClient {
   public:
    using StorageAskHandler = std::function<void(outcome::result<StorageAsk>)>;

    virtual ~StorageMarketClient() = default;

    /**
     * Initialise client instance
     */
    virtual outcome::result<void> init() = 0;

    virtual void run() = 0;

    virtual outcome::result<void> stop() = 0;

    /**
     * One round of waiting deals polling.
     *
     * Waiting deals await for terminal status from storage market provider.
     * Once the status of the deal is changed in the provider, it can be
     * retrieved by the client to proceed the deal pipeline. The function
     * performs that retrieval for all waiting deals. It should be called in
     * timer.
     */
    virtual bool pollWaiting() = 0;

    /**
     * Lists the providers in the storage market state
     * @return vector of storage provider info
     */
    virtual outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const = 0;

    /**
     * Lists all on-chain deals associated with a storage client
     * @param address
     * @return
     */
    virtual outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const = 0;

    virtual outcome::result<std::vector<ClientDeal>> listLocalDeals() const = 0;

    virtual outcome::result<ClientDeal> getLocalDeal(const CID &cid) const = 0;

    virtual void getAsk(const StorageProviderInfo &info,
                        const StorageAskHandler &storage_ask_handler) = 0;

    /**
     * Initiate deal by proposing storage deal
     * @param client_address
     * @param provider_info
     * @param data_ref
     * @param start_epoch
     * @param end_epoch
     * @param price
     * @param collateral
     * @param registered_proof
     * @return proposal CID
     */
    virtual outcome::result<CID> proposeStorageDeal(
        const Address &client_address,
        const StorageProviderInfo &provider_info,
        const DataRef &data_ref,
        const ChainEpoch &start_epoch,
        const ChainEpoch &end_epoch,
        const TokenAmount &price,
        const TokenAmount &collateral,
        const RegisteredSealProof &registered_proof,
        bool verified_deal,
        bool is_fast_retrieval) = 0;
  };

}  // namespace fc::markets::storage::client
