/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP

#include <libp2p/host/host.hpp>
#include "api/api.hpp"
#include "common/logger.hpp"
#include "data_transfer/manager.hpp"
#include "markets/pieceio/pieceio_impl.hpp"
#include "markets/storage/client/client.hpp"
#include "markets/storage/network/libp2p_storage_market_network.hpp"
#include "storage/filestore/filestore.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::markets::storage::client {

  using api::Api;
  using fc::storage::filestore::FileStore;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::keystore::KeyStore;
  using libp2p::Host;
  using network::Libp2pStorageMarketNetwork;
  using pieceio::PieceIO;

  class ClientImpl : public Client,
                     public std::enable_shared_from_this<ClientImpl> {
   public:
    ClientImpl(std::shared_ptr<Host> host,
               std::shared_ptr<boost::asio::io_context> context,
               std::shared_ptr<Api> api,
               std::shared_ptr<KeyStore> keystore,
               std::shared_ptr<PieceIO> piece_io);

    void run() override;

    void stop() override;

    outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const override;

    outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const override;

    outcome::result<std::vector<StorageDeal>> listLocalDeals() const override;

    outcome::result<StorageDeal> getLocalDeal(const CID &cid) const override;

    void getAsk(const StorageProviderInfo &info,
                const SignedAskHandler &signed_ask_handler) const override;

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
    outcome::result<SignedStorageAsk> validateAskResponse(
        const outcome::result<AskResponse> &response,
        const StorageProviderInfo &info) const;

    outcome::result<std::pair<CID, UnpaddedPieceSize>> calculateCommP(
        const RegisteredProof &registered_proof, const DataRef &data_ref) const;

    outcome::result<ClientDealProposal> signProposal(
        const Address &address, const DealProposal &proposal) const;

    /**
     * If error is present, closes connection and prints message
     * @tparam T - result type
     * @param res - result to check for error
     * @param on_error_msg - message to log on error
     * @param stream - stream to close on error
     * @param handler - error handler
     * @return true if
     */
    template <class T, class THandler>
    bool hasValue(outcome::result<T> res,
                  const std::string &on_error_msg,
                  const std::shared_ptr<CborStream> &stream,
                  const THandler &handler) const {
      if (res.has_error()) {
        logger_->error(on_error_msg + res.error().message());
        handler(res.error());
        network_->closeStreamGracefully(stream);
        return false;
      }
      return true;
    };

    /** libp2p host */
    std::shared_ptr<Host> host_;
    std::shared_ptr<boost::asio::io_context> context_;

    std::shared_ptr<Api> api_;
    std::shared_ptr<KeyStore> keystore_;
    std::shared_ptr<PieceIO> piece_io_;
    std::shared_ptr<StorageMarketNetwork> network_;

    // TODO
    // discovery

    // TODO
    // state machine

    // TODO
    // connection manager

    common::Logger logger_ = common::createLogger("StorageMarketClient");
  };

  /**
   * @brief Type of errors returned by Storage Market Client
   */
  enum class StorageMarketClientError {
    WRONG_MINER = 1,
    SIGNATURE_INVALID,
    PIECE_DATA_NOT_SET_MANUAL_TRANSFER,
    PIECE_SIZE_GREATER_SECTOR_SIZE,
    ADD_FUNDS_CALL_ERROR,
    UNKNOWN_ERROR
  };

}  // namespace fc::markets::storage::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::client,
                          StorageMarketClientError);

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_IMPL_HPP
