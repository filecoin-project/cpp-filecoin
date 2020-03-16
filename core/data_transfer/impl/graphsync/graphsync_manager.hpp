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

namespace fc::data_transfer::graphsync {

  using libp2p::Host;

  class GraphSyncManager : public Manager {
   public:
    GraphSyncManager(std::shared_ptr<Host> host, const PeerInfo &peer);

    outcome::result<ChannelId> openPushDataChannel(
        const PeerInfo &to,
        const Voucher &voucher,
        CID base_cid,
        std::shared_ptr<IPLDNode> selector) override;

    outcome::result<ChannelId> openPullDataChannel(
        const PeerInfo &to,
        const Voucher &voucher,
        CID base_cid,
        std::shared_ptr<IPLDNode> selector) override;


    outcome::result<ChannelId> createChannel(
        const TransferId &transfer_id,
        const CID &base_cid,
        std::shared_ptr<IPLDNode> selector,
        const std::vector<uint8_t> &voucher,
        const PeerInfo &initiator,
        const PeerInfo &sender_peer,
        const PeerInfo &receiver_peer) override;

    outcome::result<void> closeChannel(const ChannelId &channel_id) override;

    boost::optional<ChannelState> getChannelByIdAndSender(
        const ChannelId &channel_id, const PeerInfo &sender) override;

   private:
    /**
     * Encapsulates message creation and posting to the data transfer network
     * with the provided parameters
     * @param selector
     * @param is_pool
     * @param voucher
     * @param base_cid
     * @return transfer id
     */
    outcome::result<TransferId> sendDtRequest(
        std::shared_ptr<IPLDNode> selector,
        bool is_pull,
        const Voucher &voucher,
        const CID &base_cid,
        const PeerInfo &to);

    /**
     * Sends response
     */
    outcome::result<void> sendResponse(bool is_accepted,
                                       const PeerInfo &to,
                                       TransferId transfer_id);

    std::atomic<TransferId> last_tx_id{0};
    Libp2pDataTransferNetwork network_;
    PeerInfo peer_;
    std::map<ChannelId, ChannelState> channels_;
  };

  /**
   * @brief Type of errors returned by GraphsyncReceiver
   */
  enum class GraphsyncManagerError { STATE_ALREADY_EXISTS };

}  // namespace fc::data_transfer::graphsync

OUTCOME_HPP_DECLARE_ERROR(fc::data_transfer::graphsync, GraphsyncManagerError);

#endif  // CPP_FILECOIN_DATA_TRANSFER_GRAPHSYNC_MANAGER_HPP
