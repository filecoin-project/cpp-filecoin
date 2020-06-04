/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_DATA_TRANSFER_NETWORK_MOCK_HPP
#define CPP_FILECOIN_DATA_TRANSFER_NETWORK_MOCK_HPP

#include "data_transfer/network.hpp"

#include <gmock/gmock.h>

namespace fc::data_transfer {

  class DataTransferNetworkMock : public DataTransferNetwork {
   public:
    MOCK_METHOD1(
        setDelegate,
        outcome::result<void>(std::shared_ptr<MessageReceiver> receiver));

    MOCK_METHOD1(connectTo, outcome::result<void>(const PeerInfo &peer));

    MOCK_METHOD1(
        newMessageSender,
        outcome::result<std::shared_ptr<MessageSender>>(const PeerInfo &peer));
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_DATA_TRANSFER_NETWORK_MOCK_HPP
