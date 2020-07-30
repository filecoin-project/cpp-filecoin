/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP

#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
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
  using primitives::sector::RegisteredProof;

  class StorageMarketClient {
   public:
    using SignedAskHandler =
        std::function<void(outcome::result<SignedStorageAsk>)>;

    virtual ~StorageMarketClient() = default;

    /**
     * Initialise client instance
     */
    virtual outcome::result<void> init() = 0;

    virtual void run() = 0;

    virtual outcome::result<void> stop() = 0;

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
                        const SignedAskHandler &signed_ask_handler) = 0;

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
        const RegisteredProof &registered_proof) = 0;

    virtual outcome::result<StorageParticipantBalance> getPaymentEscrow(
        const Address &address) const = 0;

    virtual outcome::result<void> addPaymentEscrow(
        const Address &address, const TokenAmount &amount) = 0;
  };

}  // namespace fc::markets::storage::client

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP
