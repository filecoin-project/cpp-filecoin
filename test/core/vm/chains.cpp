/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

using fc::primitives::tipset::Tipset;

TEST(ChainsTest, StoreDeal) {
  auto ipld{std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  auto car{readFile(resourcePath("chain-store-deal.car"))};
  EXPECT_OUTCOME_TRUE(head, fc::storage::car::loadCar(*ipld, car));
  EXPECT_OUTCOME_TRUE(ts, Tipset::load(*ipld, head));
  std::vector<Tipset> tss;
  while (true) {
    tss.push_back(ts);
    if (ts.height == 0) {
      break;
    }
    EXPECT_OUTCOME_TRUE(parent, ts.loadParent(*ipld));
    ts = std::move(parent);
  }
  std::reverse(tss.begin(), tss.end());
  auto last{std::make_pair(tss[0].getParentStateRoot(),
                           tss[0].getParentMessageReceipts())};
  for (auto &ts : tss) {
    EXPECT_EQ(last.first, ts.getParentStateRoot())
        << "state differs at " << ts.height - 1;
    EXPECT_EQ(last.second, ts.getParentMessageReceipts())
        << "receipts differ at " << ts.height - 1;
    EXPECT_OUTCOME_TRUE(
        result, fc::vm::interpreter::InterpreterImpl{}.interpret(ipld, ts));
    last = {result.state_root, result.message_receipts};
  }
}
