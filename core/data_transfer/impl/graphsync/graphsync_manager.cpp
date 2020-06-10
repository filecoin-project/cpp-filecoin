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

  outcome::result<void> GraphSyncManager::init(
      const std::string &voucher_type,
      std::shared_ptr<RequestValidator> validator) {
    auto receiver = std::make_shared<GraphsyncReceiver>(
        network_, std::move(graphsync_), shared_from_this(), peer_);
    OUTCOME_TRY(receiver->registerVoucherType(voucher_type, validator));
    return network_->setDelegate(receiver);
  }

  outcome::result<ChannelId> GraphSyncManager::openPushDataChannel(
      const PeerInfo &to,
      const Voucher &voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    OUTCOME_TRY(transfer_id,
                sendDtRequest(selector, false, voucher, base_cid, to));
    // initiator = us, sender = us, receiver = them
    OUTCOME_TRY(
        channel_id,
        createChannel(
            transfer_id, base_cid, selector, voucher.bytes, peer_, peer_, to));
    return std::move(channel_id);
  }

  outcome::result<ChannelId> GraphSyncManager::openPullDataChannel(
      const PeerInfo &to,
      const Voucher &voucher,
      CID base_cid,
      std::shared_ptr<Selector> selector) {
    OUTCOME_TRY(transfer_id,
                sendDtRequest(selector, true, voucher, base_cid, to));
    // initiator = us, sender = them, receiver = us
    OUTCOME_TRY(
        channel_id,
        createChannel(
            transfer_id, base_cid, selector, voucher.bytes, peer_, to, peer_));
    return std::move(channel_id);
  }

  outcome::result<ChannelId> GraphSyncManager::createChannel(
      const TransferId &transfer_id,
      const CID &base_cid,
      std::shared_ptr<Selector> selector,
      const std::vector<uint8_t> &voucher,
      const PeerInfo &initiator,
      const PeerInfo &sender_peer,
      const PeerInfo &receiver_peer) {
    ChannelId channel_id{.initiator = initiator, .id = transfer_id};
    Channel channel{.transfer_id = 0,
                    .base_cid = base_cid,
                    .selector = std::move(selector),
                    .voucher = voucher,
                    .sender = sender_peer,
                    .recipient = receiver_peer,
                    .total_size = 0};
    ChannelState state{.channel = channel, .sent = 0, .received = 0};

    // TODO check thread-safety of channels_
    auto res = channels_.try_emplace(channel_id, state);
    if (!res.second) return GraphsyncManagerError::STATE_ALREADY_EXISTS;

    return channel_id;
  }

  outcome::result<void> GraphSyncManager::closeChannel(
      const ChannelId &channel_id) {
    return outcome::success();
  }

  outcome::result<TransferId> GraphSyncManager::sendDtRequest(
      const std::shared_ptr<Selector> &selector,
      bool is_pull,
      const Voucher &voucher,
      const CID &base_cid,
      const PeerInfo &to) {
    // TODO (a.chernyshov) implement selectors and serialize
    std::vector<uint8_t> selector_bytes{};

    TransferId tx_id = ++last_tx_id;
    OUTCOME_TRY(base_cid_str, base_cid.toString());
    DataTransferMessage message = createRequest(base_cid_str,
                                                is_pull,
                                                selector_bytes,
                                                voucher.bytes,
                                                voucher.type,
                                                tx_id);
    network_->sendMessage(to, message);
    return tx_id;
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

  outcome::result<void> GraphSyncManager::sendResponse(bool is_accepted,
                                                       const PeerInfo &to,
                                                       TransferId transfer_id) {
    DataTransferMessage message = createResponse(is_accepted, transfer_id);
    network_->sendMessage(to, message);
    return outcome::success();
  }

}  // namespace fc::data_transfer::graphsync

OUTCOME_CPP_DEFINE_CATEGORY(fc::data_transfer::graphsync,
                            GraphsyncManagerError,
                            e) {
  using fc::data_transfer::graphsync::GraphsyncManagerError;

  switch (e) {
    case GraphsyncManagerError::STATE_ALREADY_EXISTS:
      return "GraphsyncManagerError: state already exists";
  }

  return "unknown error";
}
