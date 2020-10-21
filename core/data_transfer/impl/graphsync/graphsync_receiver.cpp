/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_receiver.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "common/libp2p/peer/peer_info_helper.hpp"
#include "data_transfer/impl/graphsync/data_transfer_extension.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::isError;
  using storage::ipfs::graphsync::isSuccess;
  using storage::ipfs::graphsync::ResponseStatusCode;

  GraphsyncReceiver::GraphsyncReceiver(
      std::weak_ptr<DataTransferNetwork> network,
      std::shared_ptr<Graphsync> graphsync,
      std::weak_ptr<Manager> graphsync_manager,
      PeerInfo peer)
      : network_(std::move(network)),
        graphsync_(std::move(graphsync)),
        graphsync_manager_(std::move(graphsync_manager)),
        peer_(std::move(peer)) {}

  outcome::result<void> GraphsyncReceiver::receiveRequest(
      const PeerInfo &initiator, const DataTransferRequest &request) {
    if (request.type != MessageType::kNewMessage) {
      logger_->error(
          "GraphsyncReceiver::receiveRequest not implemented type {}",
          request.type);
      return sendResponse(initiator, false, request.transfer_id);
    }

    auto validated = validateVoucher(initiator, request);
    if (!validated) {
      logger_->warn("Voucher is not valid: " + validated.error().message());
      return sendResponse(initiator, false, request.transfer_id);
    }

    // TODO (a.chernyshov) implement selectors and deserialize from
    // request.selector
    auto selector{std::make_shared<Selector>(request.selector)};

    if (auto manager = graphsync_manager_.lock()) {
      if (request.is_pull) {
        auto channel = manager->createChannel(request.transfer_id,
                                              request.base_cid,
                                              selector,
                                              request.voucher->b,
                                              initiator,
                                              peer_,
                                              initiator);
        if (!channel) {
          logger_->warn("Cannot create channel: " + channel.error().message());
          return sendResponse(initiator, false, request.transfer_id);
        }
      } else {
        DataTransferResponse res{
            .type = MessageType::kCompleteMessage,
            .is_accepted = true,
            .is_pause = {},
            .transfer_id = request.transfer_id,
            .voucher = {},
            .voucher_type = {},
        };
        OUTCOME_TRY(sendGraphSyncRequest(initiator,
                                         request.transfer_id,
                                         request.is_pull,
                                         initiator,
                                         request.base_cid,
                                         res,
                                         request.selector.b));
        auto channel = manager->createChannel(request.transfer_id,
                                              request.base_cid,
                                              selector,
                                              request.voucher->b,
                                              initiator,
                                              initiator,
                                              peer_);
        if (!channel) {
          logger_->warn("Cannot create channel: " + channel.error().message());
          return sendResponse(initiator, false, request.transfer_id);
        }
      }
    }

    return sendResponse(initiator, true, request.transfer_id);
  }

  outcome::result<void> GraphsyncReceiver::receiveResponse(
      const PeerInfo &sender, const DataTransferResponse &response) {
    Event event{.code = EventCode::ERROR,
                .message = "",
                .timestamp = clock::UTCClockImpl().nowUTC()};
    if (response.is_accepted) {
      // if we are handling a response to a pull request then they are sending
      // data and the initiator is us. construct a channel id for a pull
      // request that we initiated and see if there is one in our saved
      // channel list. otherwise we should not respond
      ChannelId channel_id{.initiator = peer_, .id = response.transfer_id};
      if (auto manager = graphsync_manager_.lock()) {
        auto channel_state =
            manager->getChannelByIdAndSender(channel_id, sender);
        if (channel_state) {
          OUTCOME_TRY(sendGraphSyncRequest(
              peer_,
              response.transfer_id,
              true,
              sender,
              channel_state->channel.base_cid,
              // TODO
              {},
              // TODO (a.chernyshov) implement selectors and
              // serialize channel_state->channel.selector
              {}));
          event.code = EventCode::PROGRESS;
          notifySubscribers(event, *channel_state);
        }
      }
    }
    return outcome::success();
  }

  void GraphsyncReceiver::receiveError() {
    logger_->warn("Receive error");
  }

  outcome::result<void> GraphsyncReceiver::sendResponse(
      const PeerInfo &peer, bool is_accepted, const TransferId &transfer_id) {
    if (auto network = network_.lock()) {
      network->sendMessage(peer,
                           DataTransferResponse{MessageType::kNewMessage,
                                                is_accepted,
                                                false,
                                                transfer_id,
                                                {},
                                                {}});
    }
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
      const PeerInfo &initiator,
      const TransferId &transfer_id,
      bool is_pull,
      const PeerInfo &sender,
      const CID &root,
      DataTransferMessage message,
      gsl::span<const uint8_t> selector) {
    Extension extension{std::string{kDataTransferExtensionName}, {}};
    OUTCOME_TRYA(extension.data, codec::cbor::encode(message));

    auto _sub{std::make_shared<libp2p::protocol::Subscription>()};
    *_sub = graphsync_->makeRequest(
        sender.id,
        boost::none,
        root,
        selector,
        {extension},
        [this, initiator, transfer_id, sender, _sub](
            ResponseStatusCode code, std::vector<Extension> extensions) {
          Event event{.code = EventCode::ERROR,
                      .message = "",
                      .timestamp = clock::UTCClockImpl().nowUTC()};

          ChannelId channel_id{.initiator = initiator, .id = transfer_id};
          if (auto manager = graphsync_manager_.lock()) {
            auto channel = manager->getChannelByIdAndSender(channel_id, sender);
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
          }
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
