/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/ticket/ticket_codec.hpp"

#include <gtest/gtest.h>
#include "testutil/primitives/ticket_utils.hpp"
#include "testutil/outcome.hpp"

 struct TicketCodecTest: public ::testing::Test {
   using Ticket = fc::primitives::ticket::Ticket;

   TicketGenerator generator;
 };

 TEST_F(TicketCodecTest, EncodeDecodeSuccess) {

 }
