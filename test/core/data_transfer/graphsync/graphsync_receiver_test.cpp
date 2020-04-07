/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/graphsync_receiver.hpp"

#include <gtest/gtest.h>
#include "testutil/mocks/data_transfer/data_transfer_network_mock.hpp"
#include "testutil/mocks/data_transfer/manager_mock.hpp"
#include "testutil/mocks/data_transfer/request_validator_mock.hpp"
#include "testutil/mocks/storage/ipfs/graphsync/graphsync_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/peer_id.hpp"

namespace fc::data_transfer {

  using storage::ipfs::graphsync::GraphsyncMock;

  class GraphsyncReceiverTest : public ::testing::Test {
   public:
    std::shared_ptr<DataTransferNetworkMock> network =
        std::make_shared<DataTransferNetworkMock>();
    std::shared_ptr<GraphsyncMock> graphsync =
        std::make_shared<GraphsyncMock>();
    std::shared_ptr<ManagerMock> graphsync_manager =
        std::make_shared<ManagerMock>();
    PeerId peer_id = generatePeerId(1);

    GraphsyncReceiver receiver{network, graphsync, graphsync_manager, peer_id};

    std::shared_ptr<RequestValidatorMock> request_validator =
        std::make_shared<RequestValidatorMock>();
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
        MessageReceiverError::VOUCHER_VALIDATOR_ALREADY_REGISTERED,
        receiver.registerVoucherType(voucher_type, request_validator));
  }

}  // namespace fc::data_transfer
