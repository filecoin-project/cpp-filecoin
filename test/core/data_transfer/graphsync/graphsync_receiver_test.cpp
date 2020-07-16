/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_receiver.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/mocks/data_transfer/data_transfer_network_mock.hpp"
#include "testutil/mocks/data_transfer/manager_mock.hpp"
#include "testutil/mocks/data_transfer/message_sender_mock.hpp"
#include "testutil/mocks/data_transfer/request_validator_mock.hpp"
#include "testutil/mocks/storage/ipfs/graphsync/graphsync_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::GraphsyncMock;
  using storage::ipfs::graphsync::Subscription;
  using ::testing::_;
  using ::testing::Eq;

  class GraphsyncReceiverTest : public ::testing::Test {
   public:
    std::shared_ptr<DataTransferNetworkMock> network =
        std::make_shared<DataTransferNetworkMock>();
    std::shared_ptr<GraphsyncMock> graphsync =
        std::make_shared<GraphsyncMock>();
    std::shared_ptr<ManagerMock> graphsync_manager =
        std::make_shared<ManagerMock>();
    PeerInfo peer_info{.id = generatePeerId(1), .addresses = {}};

    GraphsyncReceiver receiver{
        network, graphsync, graphsync_manager, peer_info};

    std::shared_ptr<RequestValidatorMock> request_validator =
        std::make_shared<RequestValidatorMock>();
    PeerInfo initiator{.id = generatePeerId(2), .addresses = {}};
  };

  /**
   * @given receiver with voucher registered
   * @when try register again
   * @then got error VOUCHER_VALIDATOR_ALREADY_REGISTERED
   */
  TEST_F(GraphsyncReceiverTest, RegisterVoucher) {
    std::string voucher_type = "registered";
    EXPECT_OUTCOME_TRUE_1(
        receiver.registerVoucherType(voucher_type, request_validator));

    EXPECT_OUTCOME_ERROR(
        MessageReceiverError::kVoucherValidatorAlreadyRegistered,
        receiver.registerVoucherType(voucher_type, request_validator));
  }

  /**
   * @given receiver with voucher not registered
   * @when try validata
   * @then got response not accepted
   */
  TEST_F(GraphsyncReceiverTest, VoucherNotFound) {
    DataTransferRequest request{.base_cid = "base_cid",
                                .is_cancel = true,
                                .pid = {},
                                .is_part = false,
                                .is_pull = false,
                                .selector = {},
                                .voucher = {},
                                .voucher_type = "not_found",
                                .transfer_id = 1};
    DataTransferMessage expected_response{
        .is_request = false,
        .request = boost::none,
        .response =
            DataTransferResponse{.is_accepted = false, .transfer_id = 1}};
    EXPECT_CALL(*network, sendMessage(Eq(initiator), Eq(expected_response)))
        .WillOnce(::testing::Return());
    EXPECT_FALSE(receiver.receiveRequest(initiator, request).has_error());
  }

  /**
   * @given receiver with voucher registered
   * @when send pull request
   * @then successfull response
   */
  TEST_F(GraphsyncReceiverTest, PullRequest) {
    std::string voucher_type = "registered";
    TransferId transfer_id = 1;
    CID base_cid = "010001020005"_cid;
    EXPECT_OUTCOME_TRUE(bace_cid_str, base_cid.toString());

    DataTransferRequest request{.base_cid = bace_cid_str,
                                .is_cancel = true,
                                .pid = {},
                                .is_part = false,
                                .is_pull = true,
                                .selector = {},
                                .voucher = {},
                                .voucher_type = voucher_type,
                                .transfer_id = transfer_id};
    EXPECT_CALL(*graphsync_manager,
                createChannel(Eq(transfer_id),
                              Eq(base_cid),
                              _,
                              _,
                              Eq(initiator),
                              Eq(peer_info),
                              Eq(initiator)))
        .WillOnce(::testing::Return(outcome::success(
            ChannelId{.initiator = initiator, .id = transfer_id})));
    EXPECT_CALL(*request_validator, validatePull(Eq(initiator), _, base_cid, _))
        .WillOnce(::testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(
        receiver.registerVoucherType(voucher_type, request_validator));

    DataTransferMessage expected_response{
        .is_request = false,
        .request = boost::none,
        .response = DataTransferResponse{.is_accepted = true,
                                         .transfer_id = transfer_id}};

    EXPECT_CALL(*network, sendMessage(Eq(initiator), Eq(expected_response)))
        .WillOnce(::testing::Return());
    EXPECT_FALSE(receiver.receiveRequest(initiator, request).has_error());
  }

  /**
   * @given receiver with voucher registered
   * @when send push request
   * @then successfull response
   */
  TEST_F(GraphsyncReceiverTest, PushRequest) {
    std::string voucher_type = "registered";
    TransferId transfer_id = 1;
    CID base_cid = "010001020005"_cid;
    EXPECT_OUTCOME_TRUE(bace_cid_str, base_cid.toString());

    DataTransferRequest request{.base_cid = bace_cid_str,
                                .is_cancel = true,
                                .pid = {},
                                .is_part = false,
                                .is_pull = false,
                                .selector = {},
                                .voucher = {},
                                .voucher_type = voucher_type,
                                .transfer_id = transfer_id};
    EXPECT_CALL(*graphsync_manager,
                createChannel(Eq(transfer_id),
                              Eq(base_cid),
                              _,
                              _,
                              Eq(initiator),
                              Eq(initiator),
                              Eq(peer_info)))
        .WillOnce(::testing::Return(outcome::success(
            ChannelId{.initiator = initiator, .id = transfer_id})));
    EXPECT_CALL(*graphsync,
                makeRequest(Eq(initiator.id), _, Eq(base_cid), _, _, _));

    EXPECT_CALL(*request_validator, validatePush(Eq(initiator), _, base_cid, _))
        .WillOnce(::testing::Return(outcome::success()));

    EXPECT_OUTCOME_TRUE_1(
        receiver.registerVoucherType(voucher_type, request_validator));

    DataTransferMessage expected_response{
        .is_request = false,
        .request = boost::none,
        .response = DataTransferResponse{.is_accepted = true,
                                         .transfer_id = transfer_id}};
    EXPECT_CALL(*network, sendMessage(Eq(initiator), Eq(expected_response)))
        .WillOnce(::testing::Return());
    EXPECT_FALSE(receiver.receiveRequest(initiator, request).has_error());
  }

}  // namespace fc::data_transfer
