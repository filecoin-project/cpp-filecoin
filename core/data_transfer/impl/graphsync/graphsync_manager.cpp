/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_manager.hpp"

#include <utility>
#include "data_transfer/impl/graphsync/graphsync_receiver.hpp"
#include "data_transfer/message.hpp"

namespace fc::data_transfer::graphsync {

  using common::Buffer;

  GraphSyncManager::GraphSyncManager(std::shared_ptr<Host> host,
                                     std::shared_ptr<Graphsync> graphsync)
      : peer_{host->getPeerInfo()},
        network_(std::make_shared<Libp2pDataTransferNetwork>(std::move(host))),
        graphsync_(std::move(graphsync)) {}

  void GraphSyncManager::subscribe(
      const std::shared_ptr<Subscriber> &subscriber) {
    receiver->subscribeToEvents(subscriber);
  }

  outcome::result<void> GraphSyncManager::init(
      const std::string &voucher_type,
      std::shared_ptr<RequestValidator> validator) {
    receiver = std::make_shared<GraphsyncReceiver>(
        network_, graphsync_, weak_from_this(), peer_);
    OUTCOME_TRY(receiver->registerVoucherType(voucher_type, validator));
    return network_->setDelegate(receiver);
  }

  outcome::result<ChannelId> GraphSyncManager::openDataChannel(
      const PeerInfo &to,
      bool pull,
      const Voucher &voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    auto transfer_id{++last_tx_id};
    OUTCOME_TRY(channel_id,
                createChannel(transfer_id,
                              base_cid,
                              selector,
                              voucher.bytes,
                              peer_,
                              pull ? to : peer_,
                              pull ? peer_ : to));
    assert(!voucher.bytes.empty());
    network_->sendMessage(to,
                          DataTransferRequest{base_cid,
                                              MessageType::kNewMessage,
                                              false,
                                              false,
                                              pull,
                                              *selector,
                                              CborRaw{Buffer{voucher.bytes}},
                                              voucher.type,
                                              transfer_id});
    return std::move(channel_id);
  }

  outcome::result<ChannelId> GraphSyncManager::createChannel(
      const TransferId &transfer_id,
      const CID &base_cid,
      std::shared_ptr<Selector> selector,
      BytesIn voucher,
      const PeerInfo &initiator,
      const PeerInfo &sender_peer,
      const PeerInfo &receiver_peer) {
    ChannelId channel_id{.initiator = initiator, .id = transfer_id};
    Channel channel{.transfer_id = 0,
                    .base_cid = base_cid,
                    .selector = std::move(selector),
                    .voucher = Buffer{voucher},
                    .sender = sender_peer,
                    .recipient = receiver_peer,
                    .total_size = 0};
    ChannelState state{.channel = channel, .sent = 0, .received = 0};

    // TODO check thread-safety of channels_
    auto res = channels_.try_emplace(channel_id, state);
    if (!res.second) return GraphsyncManagerError::kStateAlreadyExists;

    return channel_id;
  }

  outcome::result<void> GraphSyncManager::closeChannel(
      const ChannelId &channel_id) {
    return outcome::success();
  }

  boost::optional<ChannelState> GraphSyncManager::getChannelByIdAndSender(
      const ChannelId &channel_id, const PeerInfo &sender) {
    // TODO check thread-safety of channels_
    auto found = channels_.find(channel_id);
    if (found == channels_.end() || found->second.channel.sender != sender) {
      return boost::none;
    }

    return found->second;
  }
}  // namespace fc::data_transfer::graphsync

OUTCOME_CPP_DEFINE_CATEGORY(fc::data_transfer::graphsync,
                            GraphsyncManagerError,
                            e) {
  using fc::data_transfer::graphsync::GraphsyncManagerError;

  switch (e) {
    case GraphsyncManagerError::kStateAlreadyExists:
      return "GraphsyncManagerError: state already exists";
  }

  return "unknown error";
}
