/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/impl/graphsync/data_transfer_extension.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::storage::ipfs::graphsync::Extension;

namespace fc::data_transfer {

  /**
   * @given data trasfer extension with wrong name
   * @when decode extension
   * @then error UNEXPECTED_EXTENSION_NAME
   */
  TEST(DataTransferExtensionTest, WrongExtensionName) {
    Extension wrong_extenstion{.name = "wrong_name"};
    EXPECT_OUTCOME_ERROR(DataTransferExtensionError::kUnexpectedExtensionName,
                         decodeDataTransferExtension(wrong_extenstion));
  }

  /**
   * @given extension data and go encoded bytes
   * @when ncode and decode extension data
   * @then encoding is equal to go-data-transfer
   */
  TEST(DataTransferExtensionTest, Encoding) {
    ExtensionDataTransferData extension_data{
        .transfer_id = 1, .initiator = "initiator", .is_pull = true};

    // bytes from go-data-transfer
    auto expected_data = "830169696e69746961746f72f5"_unhex;
    Extension expected{.name = std::string(kDataTransferExtensionName),
                       .data = expected_data};

    EXPECT_OUTCOME_EQ(encodeDataTransferExtension(extension_data), expected);
    EXPECT_OUTCOME_TRUE(decoded, decodeDataTransferExtension(expected));
    EXPECT_OUTCOME_EQ(encodeDataTransferExtension(decoded), expected);
  }

}  // namespace fc::data_transfer
