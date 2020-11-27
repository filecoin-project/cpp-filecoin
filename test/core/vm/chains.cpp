/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/actor/cgo/cgo_invoker.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"

using fc::primitives::StoragePower;
using fc::primitives::sector::RegisteredProof;
using fc::primitives::tipset::Tipset;
using fc::primitives::tipset::TipsetCPtr;
using fc::vm::actor::Invoker;
using fc::vm::actor::cgo::CgoInvoker;
using fc::vm::interpreter::InterpreterImpl;
using fc::vm::runtime::TipsetRandomness;

TEST(ChainsTest, Testnet_v054_h341) {
  spdlog::info("loading");
  fc::vm::actor::cgo::config(StoragePower{1} << 20,
                             StoragePower{10} << 40,
                             {RegisteredProof::StackedDRG32GiBSeal,
                              RegisteredProof::StackedDRG64GiBSeal});
  auto ipld{std::make_shared<fc::storage::ipfs::InMemoryDatastore>()};
  auto car{readFile(resourcePath("testnet341.car"))};
  EXPECT_OUTCOME_TRUE(head, fc::storage::car::loadCar(*ipld, car));
  EXPECT_OUTCOME_TRUE(ts, Tipset::load(*ipld, head));
  std::vector<TipsetCPtr> tss;
  while (true) {
    tss.push_back(ts);
    if (ts->height() == 0) {
      break;
    }
    EXPECT_OUTCOME_TRUE(parent, ts->loadParent(*ipld));
    ts = std::move(parent);
  }
  std::reverse(tss.begin(), tss.end());
  auto last{std::make_pair(tss[0]->getParentStateRoot(),
                           tss[0]->getParentMessageReceipts())};
  for (auto &ts : tss) {
    spdlog::info("validating height {}", ts->height());
    EXPECT_EQ(last.first, ts->getParentStateRoot())
        << "state differs at " << ts->height() - 1;
    EXPECT_EQ(last.second, ts->getParentMessageReceipts())
        << "receipts differ at " << ts->height() - 1;
    std::shared_ptr<Invoker> invoker = std::make_shared<CgoInvoker>(false);
    auto randomness = std::make_shared<TipsetRandomness>(ipld, ts);
    EXPECT_OUTCOME_TRUE(
        result, (InterpreterImpl{invoker, randomness}.interpret(ipld, ts)));
    last = {result.state_root, result.message_receipts};
  }
  spdlog::info("done");
}
