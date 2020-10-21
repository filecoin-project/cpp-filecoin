/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_MANAGER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_MANAGER_HPP

#include "data_transfer/manager.hpp"

#include <libp2p/host/host.hpp>
#include "data_transfer/impl/libp2p_data_transfer_network.hpp"
#include "data_transfer/types.hpp"
#include "node/fwd.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::data_transfer::graphsync {

  using libp2p::Host;
  using storage::ipfs::graphsync::Graphsync;

  class GraphSyncManager
      : public Manager,
        public std::enable_shared_from_this<GraphSyncManager> {
   public:
    GraphSyncManager(std::shared_ptr<Host> host,
                     std::shared_ptr<Graphsync> graphsync);

    void subscribe(const std::shared_ptr<Subscriber> &subscriber) override;

    outcome::result<void> init(
        const std::string &voucher_type,
        std::shared_ptr<RequestValidator> validator) override;

    outcome::result<ChannelId> openDataChannel(
        const PeerInfo &to,
        bool pull,
        const Voucher &voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) override;

    outcome::result<ChannelId> createChannel(
        const TransferId &transfer_id,
        const CID &base_cid,
        std::shared_ptr<Selector> selector,
        BytesIn voucher,
        const PeerInfo &initiator,
        const PeerInfo &sender_peer,
        const PeerInfo &receiver_peer) override;

    outcome::result<void> closeChannel(const ChannelId &channel_id) override;

    boost::optional<ChannelState> getChannelByIdAndSender(
        const ChannelId &channel_id, const PeerInfo &sender) override;

   private:
    std::atomic<TransferId> last_tx_id{0};
    PeerInfo peer_;
    std::shared_ptr<Libp2pDataTransferNetwork> network_;
    std::shared_ptr<Graphsync> graphsync_;
    std::map<ChannelId, ChannelState> channels_;
    std::shared_ptr<GraphsyncReceiver> receiver;
  };

  /**
   * @brief Type of errors returned by GraphsyncReceiver
   */
  enum class GraphsyncManagerError { kStateAlreadyExists = 1 };

}  // namespace fc::data_transfer::graphsync

OUTCOME_HPP_DECLARE_ERROR(fc::data_transfer::graphsync, GraphsyncManagerError);

#endif  // CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_MANAGER_HPP
