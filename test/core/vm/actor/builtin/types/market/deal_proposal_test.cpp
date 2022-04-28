/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::vm::actor::builtin::types::market {
  /**
   * @given bytes from go specs-actors
   * @when encode and decode
   * @then expected and actual are the same
   */
  TEST(DealInfoManagerTest, LabelCodec) {
    Label emptyLabel;
    expectEncodeAndReencode(emptyLabel, "60"_unhex);

    Label bytesLabel = Label::make(Bytes{0xca, 0xfe, 0xb0, 0x0a}).value();
    expectEncodeAndReencode(bytesLabel, "44cafeb00a"_unhex);

    Label stringLabel =
        Label::make("i am a label, turn me into cbor maj typ 3 plz").value();
    expectEncodeAndReencode(
        stringLabel,
        "782d6920616d2061206c6162656c2c207475726e206d6520696e746f2063626f72206d616a20747970203320706c7a"_unhex);
  }

  /**
   * @given invalid UTF-8
   * @when try create Label
   * @then error is raised
   */
  TEST(DealInfoManagerTest, LabelCodecInvalidUtf8) {
    EXPECT_OUTCOME_ERROR(
        ERROR_TEXT("Label must be a valid UTF-8 or byte array."),
        Label::make("invalid utf8: \xc3\x28"));
  }
}  // namespace fc::vm::actor::builtin::types::market
