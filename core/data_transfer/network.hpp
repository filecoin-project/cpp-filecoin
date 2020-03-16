/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_NETWORK_HPP
#define CPP_FILECOIN_DATA_TRANSFER_NETWORK_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/cbor.hpp"
#include "data_transfer/impl/stream_message_sender.hpp"
#include "data_transfer/message.hpp"
#include "data_transfer/message_receiver.hpp"
#include "data_transfer/message_sender.hpp"

namespace fc::data_transfer {

  using libp2p::peer::PeerInfo;

  /** The protocol identifier for graphsync messages */
  const libp2p::peer::Protocol kDataTransferLibp2pProtocol =
      "/fil/datatransfer/1.0.0";

  /**
   * DataTransferNetwork provides interface for network connectivity for
   * GraphSync
   */
  class DataTransferNetwork {
   public:
    virtual ~DataTransferNetwork() = default;

    /**
     * Registers the Receiver to handle messages received from the network
     */
    virtual void setDelegate(std::shared_ptr<MessageReceiver> receiver) = 0;

    /**
     * Establishes a connection to the given peer
     */
    virtual outcome::result<void> connectTo(const PeerInfo &peer) = 0;

    virtual outcome::result<std::shared_ptr<MessageSender>> newMessageSender(
        const PeerInfo &peer) = 0;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_NETWORK_HPP
