/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTILS_MOCK_MANAGER_MOCK_HPP
#define CPP_FILECOIN_TESTUTILS_MOCK_MANAGER_MOCK_HPP

#include "data_transfer/manager.hpp"

#include <gmock/gmock.h>

namespace fc::data_transfer {

  class ManagerMock : public Manager {
   public:
    MOCK_METHOD1(subscribe, void(std::weak_ptr<Subscriber>));

    MOCK_METHOD2(
        init,
        outcome::result<void>(const std::string &voucher_type,
                              std::shared_ptr<RequestValidator> validator));

    MOCK_METHOD5(
        openDataChannel,
        outcome::result<ChannelId>(const PeerInfo &to,
                                   bool,
                                   const Voucher &voucher,
                                   CID base_cid,
                                   std::shared_ptr<Selector> selector));

    MOCK_METHOD7(createChannel,
                 outcome::result<ChannelId>(const TransferId &transfer_id,
                                            const CID &base_cid,
                                            std::shared_ptr<Selector> selector,
                                            BytesIn voucher,
                                            const PeerInfo &initiator,
                                            const PeerInfo &sender_peer,
                                            const PeerInfo &receiver_peer));

    MOCK_METHOD1(closeChannel,
                 outcome::result<void>(const ChannelId &channel_id));

    MOCK_METHOD2(getChannelByIdAndSender,
                 boost::optional<ChannelState>(const ChannelId &channel_id,
                                               const PeerInfo &sender));
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_TESTUTILS_MOCK_MANAGER_MOCK_HPP
