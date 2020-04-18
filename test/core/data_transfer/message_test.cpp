/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/message.hpp"

#include <gtest/gtest.h>
#include "data_transfer/types.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"
#include "vm/actor/actor.hpp"

using fc::CID;
using fc::common::Buffer;
using fc::data_transfer::createRequest;
using fc::data_transfer::createResponse;
using fc::data_transfer::DataTransferMessage;
using fc::data_transfer::DataTransferRequest;
using fc::data_transfer::TransferId;

/**
 * @given Request message and encoded message from go-data-transfer
 * @when serialize and deserialize
 * @then results are equal
 */
TEST(Message, CborRequestMessage) {
  TransferId transfer_id{12};
  bool is_pull = false;
  std::string voucher_type = "FakeVoucherType";
  std::vector<uint8_t> voucher{4, 8, 15, 16, 23, 42};

  std::vector<uint8_t> kBuffer{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                               'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                               'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
                               'y', 'z', '1', '2', '3', '4', '5', '6'};

  EXPECT_OUTCOME_TRUE(
      multihash,
      libp2p::multi::Multihash::create(libp2p::multi::sha256, kBuffer));
  CID base_cid{
      CID::Version::V1, libp2p::multi::MulticodecType::Code::DAG_PB, multihash};
  EXPECT_OUTCOME_TRUE(cid_str, base_cid.toString());

  std::vector<uint8_t> selector{4, 8, 15, 16, 23, 42};

  DataTransferMessage message = createRequest(
      cid_str, is_pull, selector, voucher, voucher_type, transfer_id);

  auto expected_from_go =
      "83f589783b6261667962656964626d6a7277697a6c676d3575677332746c6e72777734"
      "3333716f667a68673564766f7a337871366c3267657a64676e62766779f440f4f44604"
      "080f10172a4604080f10172a6f46616b65566f7563686572547970650cf6"_unhex;

  expectEncodeAndReencode(message, expected_from_go);
}

/**
 * @given Response message and encoded message from go-data-transfer
 * @when serialize and deserialize
 * @then results are equal
 */
TEST(Message, CborResponseMessage) {
  bool is_accepted = true;
  TransferId transfer_id{12};

  DataTransferMessage message = createResponse(is_accepted, transfer_id);

  auto expected_from_go = "83f4f682f50c"_unhex;

  expectEncodeAndReencode(message, expected_from_go);
}
