/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP

#include "api/api.hpp"
#include "markets/storage/client/client.hpp"
#include "markets/storage/storage_market_network.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::markets::storage {

  using api::Api;
  using fc::storage::filestore::FileStore;
  using fc::storage::ipfs::IpfsDatastore;

  class ClientImpl : public Client {
   public:
    ClientImpl(std::shared_ptr<Api> api,
               std::shared_ptr<StorageMarketNetwork> network,
               std::shared_ptr<IpfsDatastore> block_store,
               std::shared_ptr<FileStore> file_store);

    void run() override;

    void stop() override;

    outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const override;

    outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const override;

    outcome::result<std::vector<StorageDeal>> listLocalDeals() const override;

    outcome::result<StorageDeal> getLocalDeal(const CID &cid) const override;

    outcome::result<SignedStorageAsk> getAsk(
        const StorageProviderInfo &info) const override;

    outcome::result<ProposeStorageDealResult> proposeStorageDeal(
        const Address &address,
        const StorageProviderInfo &provider_info,
        const DataRef &data_ref,
        const ChainEpoch &start_epoch,
        const ChainEpoch &end_epoch,
        const TokenAmount &price,
        const TokenAmount &collateral,
        const RegisteredProof &registered_proof) override;

    outcome::result<StorageParticipantBalance> getPaymentEscrow(
        const Address &address) const override;

    outcome::result<void> addPaymentEscrow(const Address &address,
                                           const TokenAmount &amount) override;

   private:
    std::shared_ptr<Api> api_;
    std::shared_ptr<StorageMarketNetwork> network_;

    // TODO
    // data_transfer_manager_

    std::shared_ptr<IpfsDatastore> block_store_;

    std::shared_ptr<FileStore> file_store_;

    // TODO
    // PieceIO

    // TODO
    // discovery

    Api node;

    // TODO
    // state machine

    // TODO
    // connection manager
  };

  /**
   * @brief Type of errors returned by Storage Market Client
   */
  enum class StorageMarketClientError { WRONG_MINER = 1, UNKNOWN_ERROR };

}  // namespace fc::markets::storage

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage, StorageMarketClientError);

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
