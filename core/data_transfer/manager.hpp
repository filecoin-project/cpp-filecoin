/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_MANAGER_HPP
#define CPP_FILECOIN_DATA_TRANSFER_MANAGER_HPP

#include <map>

#include <libp2p/peer/peer_info.hpp>
#include "data_transfer/request_validator.hpp"
#include "data_transfer/types.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerInfo;
  using storage::ipld::Selector;

  /**
   * Manager is the core interface presented by all implementations of of
   * the data transfer subsystem
   */
  class Manager {
   public:
    virtual ~Manager() = default;

    virtual void subscribe(std::weak_ptr<Subscriber> subscriber) = 0;

    virtual outcome::result<void> init(
        const std::string &voucher_type,
        std::shared_ptr<RequestValidator> validator) = 0;

    virtual outcome::result<ChannelId> openDataChannel(
        const PeerInfo &to,
        bool pull,
        const Voucher &voucher,
        CID base_cid,
        std::shared_ptr<Selector> selector) = 0;

    /**
     * Creates a new channel id and channel state and saves to channels
     * @return error if the channel exists already
     */
    virtual outcome::result<ChannelId> createChannel(
        const TransferId &transfer_id,
        const CID &base_cid,
        std::shared_ptr<Selector> selector,
        BytesIn voucher,
        const PeerInfo &initiator,
        const PeerInfo &sender_peer,
        const PeerInfo &receiver_peer) = 0;

    /**
     * Closes an open channel (effectively a cancel)
     */
    virtual outcome::result<void> closeChannel(const ChannelId &channel_id) = 0;

    virtual boost::optional<ChannelState> getChannelByIdAndSender(
        const ChannelId &channel_id, const PeerInfo &sender) = 0;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_MANAGER_HPP
