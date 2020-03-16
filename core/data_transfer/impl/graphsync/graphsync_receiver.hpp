/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_RECEIVER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_RECEIVER_HPP

#include "data_transfer/message_receiver.hpp"

#include "data_transfer/impl/graphsync/graphsync_manager.hpp"
#include "data_transfer/impl/libp2p_data_transfer_network.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::Graphsync;

  class GraphsyncReceiver : public MessageReceiver {
   public:
    GraphsyncReceiver(std::shared_ptr<Libp2pDataTransferNetwork> network,
                      std::shared_ptr<Graphsync> graphsync,
                      std::shared_ptr<Manager> graphsync_manager,
                      PeerInfo peer);

    outcome::result<void> receiveRequest(
        const PeerInfo &initiator, const DataTransferRequest &request) override;

    void receiveResponse(const PeerInfo &sender,
                         const DataTransferResponse &response) override;

    void receiveError() override;

   private:
    outcome::result<void> sendResponse(const PeerInfo &peer,
                                       bool is_accepted,
                                       const TransferId &transfer_id);

    void subscribeToEvents(const Subscriber &subscriber);

    void unsubscribe(const Subscriber &subscriber);

    /**
     * Assembles a graphsync request and determines if the transfer was
     * completed/successful. Notifies subscribers of final request status
     * @return
     */
    outcome::result<void> sendGraphSyncRequest(
        const PeerInfo &initiator,
        const TransferId &transfer_id,
        bool is_pull,
        const PeerInfo &sender,
        const CID &root,
        gsl::span<const uint8_t> selector);

    void notifySubscribers(const Event &event,
                           const ChannelState &channel_state);

    std::shared_ptr<Libp2pDataTransferNetwork> network_;
    std::shared_ptr<Graphsync> graphsync_;
    std::shared_ptr<Manager> graphsync_manager_;
    PeerInfo peer_;
    std::vector<Subscriber> subscribers_;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_RECEIVER_HPP
