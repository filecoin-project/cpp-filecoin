/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_LIBP2P_DATA_TRANSFER_NETWORK_HPP
#define CPP_FILECOIN_DATA_TRANSFER_LIBP2P_DATA_TRANSFER_NETWORK_HPP

#include "data_transfer/network.hpp"

#include <libp2p/host/host.hpp>

namespace fc::data_transfer {

  using libp2p::Host;

  class Libp2pDataTransferNetwork : public DataTransferNetwork {
   public:
    explicit Libp2pDataTransferNetwork(std::shared_ptr<Host> host);

    outcome::result<void> setDelegate(
        std::shared_ptr<MessageReceiver> receiver) override;

    outcome::result<void> connectTo(const PeerInfo &peer) override;

    outcome::result<std::shared_ptr<MessageSender>> newMessageSender(
        const PeerInfo &peer) override;

   private:
    std::shared_ptr<libp2p::Host> host_;
    std::shared_ptr<MessageReceiver> receiver_;
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_LIBP2P_DATA_TRANSFER_NETWORK_HPP
