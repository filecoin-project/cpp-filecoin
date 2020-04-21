/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/client_impl.hpp"

namespace fc::markets::storage {

  ClientImpl::ClientImpl(std::shared_ptr<Api> api,
                         std::shared_ptr<StorageMarketNetwork> network,
                         std::shared_ptr<IpfsDatastore> block_store,
                         std::shared_ptr<FileStore> file_store)
      : api_{std::move(api)},
        network_(std::move(network)),
        block_store_(std::move(block_store)),
        file_store_(std::move(file_store)) {}

  void ClientImpl::run() {}

  void ClientImpl::stop() {}

  outcome::result<std::vector<StorageProviderInfo>> ClientImpl::listProviders()
      const {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<std::vector<StorageDeal>> ClientImpl::listDeals(
      const Address &address) const {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<std::vector<StorageDeal>> ClientImpl::listLocalDeals() const {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<StorageDeal> ClientImpl::getLocalDeal(const CID &cid) const {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<SignedStorageAsk> ClientImpl::getAsk(
      const StorageProviderInfo &info) const {
    OUTCOME_TRY(stream, network_->newAskStream(info.peer_id));
    AskRequest request{.miner = info.address};
    OUTCOME_TRY(stream->writeAskRequest(request));
    OUTCOME_TRY(response, stream->readAskResponse());
    if (response.ask.ask.miner != info.address) {
      return StorageMarketClientError::WRONG_MINER;
    }

    OUTCOME_TRY(tipset, node.ChainHead());
    OUTCOME_TRY(tipset_key, tipset.makeKey());

    OUTCOME_TRY(worker, node.StateMinerWorker(info.worker, tipset_key));
    //    response.ask.signature
    // TODO
    // validate signature

    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<ProposeStorageDealResult> ClientImpl::proposeStorageDeal(
      const Address &address,
      const StorageProviderInfo &provider_info,
      const DataRef &data_ref,
      const ChainEpoch &start_epoch,
      const ChainEpoch &end_epoch,
      const TokenAmount &price,
      const TokenAmount &collateral,
      const RegisteredProof &registered_proof) {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<StorageParticipantBalance> ClientImpl::getPaymentEscrow(
      const Address &address) const {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

  outcome::result<void> ClientImpl::addPaymentEscrow(
      const Address &address, const TokenAmount &amount) {
    // TODO
    return StorageMarketClientError::UNKNOWN_ERROR;
  }

}  // namespace fc::markets::storage

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage, StorageMarketClientError, e) {
  using fc::markets::storage::StorageMarketClientError;

  switch (e) {
    case StorageMarketClientError::WRONG_MINER:
      return "StorageMarketClientError: wrong miner address";
    case StorageMarketClientError::UNKNOWN_ERROR:
      return "StorageMarketClientError: unknown error";
  }

  return "StorageMarketClientError: unknown error";
}
