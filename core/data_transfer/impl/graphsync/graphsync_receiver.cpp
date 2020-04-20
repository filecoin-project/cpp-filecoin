/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_receiver.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "data_transfer/impl/graphsync/data_transfer_extension.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::isError;
  using storage::ipfs::graphsync::isSuccess;
  using storage::ipfs::graphsync::ResponseStatusCode;

  GraphsyncReceiver::GraphsyncReceiver(
      std::shared_ptr<DataTransferNetwork> network,
      std::shared_ptr<Graphsync> graphsync,
      std::shared_ptr<Manager> graphsync_manager,
      PeerId peer)
      : network_(std::move(network)),
        graphsync_(std::move(graphsync)),
        graphsync_manager_(std::move(graphsync_manager)),
        peer_(std::move(peer)) {}

  outcome::result<void> GraphsyncReceiver::receiveRequest(
      const PeerId &initiator, const DataTransferRequest &request) {
    auto validated = validateVoucher(initiator, request);
    if (!validated) {
      logger_->warn("Voucher is not valid: " + validated.error().message());
      return sendResponse(initiator, false, request.transfer_id);
    }

    OUTCOME_TRY(selector, IPLDNodeImpl::createFromRawBytes(request.selector));
    OUTCOME_TRY(base_cid, CID::fromString(request.base_cid));

    if (request.is_pull) {
      auto channel = graphsync_manager_->createChannel(request.transfer_id,
                                                       base_cid,
                                                       selector,
                                                       request.voucher,
                                                       initiator,
                                                       peer_,
                                                       initiator);
      if (!channel) {
        logger_->warn("Cannot create channel: " + channel.error().message());
        return sendResponse(initiator, false, request.transfer_id);
      }
    } else {
      OUTCOME_TRY(sendGraphSyncRequest(initiator,
                                       request.transfer_id,
                                       request.is_pull,
                                       initiator,
                                       base_cid,
                                       request.selector));

      auto channel = graphsync_manager_->createChannel(request.transfer_id,
                                                       base_cid,
                                                       selector,
                                                       request.voucher,
                                                       initiator,
                                                       initiator,
                                                       peer_);
      if (!channel) {
        logger_->warn("Cannot create channel: " + channel.error().message());
        return sendResponse(initiator, false, request.transfer_id);
      }
    }

    return sendResponse(initiator, true, request.transfer_id);
  }

  outcome::result<void> GraphsyncReceiver::receiveResponse(
      const PeerId &sender, const DataTransferResponse &response) {
    Event event{.code = EventCode::ERROR,
                .message = "",
                .timestamp = clock::UTCClockImpl().nowUTC()};
    if (response.is_accepted) {
      // if we are handling a response to a pull request then they are sending
      // data and the initiator is us. construct a channel id for a pull request
      // that we initiated and see if there is one in our saved channel list.
      // otherwise we should not respond
      ChannelId channel_id{.initiator = peer_, .id = response.transfer_id};
      auto channel_state =
          graphsync_manager_->getChannelByIdAndSender(channel_id, sender);
      if (channel_state) {
        OUTCOME_TRY(sendGraphSyncRequest(
            peer_,
            response.transfer_id,
            true,
            sender,
            channel_state->channel.base_cid,
            channel_state->channel.selector->getRawBytes()));
        event.code = EventCode::PROGRESS;
        notifySubscribers(event, *channel_state);
      }
    }
    return outcome::success();
  }

  void GraphsyncReceiver::receiveError() {
    logger_->warn("Receive Error");
  }

  outcome::result<void> GraphsyncReceiver::sendResponse(
      const PeerId &peer, bool is_accepted, const TransferId &transfer_id) {
    DataTransferMessage response = createResponse(is_accepted, transfer_id);
    OUTCOME_TRY(sender, network_->newMessageSender(peer));
    OUTCOME_TRY(sender->sendMessage(response));
    return outcome::success();
  }

  void GraphsyncReceiver::subscribeToEvents(
      const std::shared_ptr<Subscriber> &subscriber) {
    subscribers_.push_back(subscriber);
  }

  void GraphsyncReceiver::unsubscribe(
      const std::shared_ptr<Subscriber> &subscriber) {
    subscribers_.erase(
        std::remove(subscribers_.begin(), subscribers_.end(), subscriber),
        subscribers_.end());
  }

  outcome::result<void> GraphsyncReceiver::sendGraphSyncRequest(
      const PeerId &initiator,
      const TransferId &transfer_id,
      bool is_pull,
      const PeerId &sender,
      const CID &root,
      gsl::span<const uint8_t> selector) {
    ExtensionDataTransferData extension_data{.transfer_id = transfer_id,
                                             .initiator = initiator.toBase58(),
                                             .is_pull = is_pull};
    OUTCOME_TRY(extension, encodeDataTransferExtension(extension_data));

    graphsync_->makeRequest(
        sender,
        boost::none,
        root,
        selector,
        {extension},
        [this, initiator, transfer_id, sender](
            ResponseStatusCode code, std::vector<Extension> extensions) {
          Event event{.code = EventCode::ERROR,
                      .message = "",
                      .timestamp = clock::UTCClockImpl().nowUTC()};

          ChannelId channel_id{.initiator = initiator, .id = transfer_id};
          auto channel = this->graphsync_manager_->getChannelByIdAndSender(
              channel_id, sender);
          if (!channel) {
            event.code = EventCode::ERROR;
            event.message = "cannot find a matching channel for this request";
          } else if (isError(code)) {
            event.code = EventCode::ERROR;
            event.message = statusCodeToString(code);
          } else if (isSuccess(code)) {
            event.code = EventCode::COMPLETE;
          }

          this->notifySubscribers(event, *channel);
        });
    return outcome::success();
  }

  void GraphsyncReceiver::notifySubscribers(const Event &event,
                                            const ChannelState &channel_state) {
    for (auto &subscriber : subscribers_) {
      subscriber->notify(event, channel_state);
    }
  }

}  // namespace fc::data_transfer
