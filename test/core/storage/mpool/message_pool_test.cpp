/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "cbor_blake/ipld_any.hpp"
#include "storage/car/car.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/mpool/mpool.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/interpreter/interpreter.hpp"

#define AUTO(l, ...)        \
  decltype(__VA_ARGS__) l { \
    __VA_ARGS__             \
  }

namespace fc::storage::mpool {
  auto tipsetMessages(IpldPtr ipld, TipsetCPtr ts) {
    std::vector<SignedMessage> msgs;
    ts->visitMessages(
          {ipld, false, true},
          [&](auto, auto bls, auto &, auto smsg, auto msg) {
            if (bls) {
              msgs.push_back({*msg, crypto::signature::BlsSignature{}});
            } else {
              msgs.push_back(*smsg);
            }
            return outcome::success();
          })
        .value();
    return msgs;
  }

  auto ipld{std::make_shared<ipfs::InMemoryDatastore>()};
  auto ts_load{std::make_shared<primitives::tipset::TsLoadIpld>(ipld)};
  auto car_roots{car::loadCar(*ipld, resourcePath("mpool.car")).value()};
  auto ts0{ts_load->load(*TipsetKey::make(car_roots)).value()};
  auto msgs0{tipsetMessages(ipld, ts0)};
  auto ts1{ts_load->load(ts0->getParents()).value()};
  auto msgs1{tipsetMessages(ipld, ts1)};
  auto ts2{ts_load->load(ts1->getParents()).value()};

  auto interpreter_cache{std::make_shared<vm::interpreter::InterpreterCache>(
      std::make_shared<InMemoryStorage>(),
      std::make_shared<AnyAsCbIpld>(ipld))};
  void cacheParentState(TipsetCPtr ts) {
    interpreter_cache->set(
        ts->getParents(),
        {ts->getParentStateRoot(), ts->getParentMessageReceipts(), {}});
  }

  struct Fixture {
    struct ChainStore : blockchain::ChainStore {
      outcome::result<void> addBlock(
          const primitives::block::BlockHeader &) override {
        throw "unused";
      }
      TipsetCPtr heaviestTipset() const override {
        throw "unused";
      }
      boost::signals2::signal<HeadChangeSignature> signal;
      connection_t subscribeHeadChanges(
          const std::function<HeadChangeSignature> &subscriber) override {
        return signal.connect(subscriber);
      }
      primitives::BigInt getHeaviestWeight() const override {
        throw "unused";
      }
    };

    AUTO(chain_store, std::make_shared<ChainStore>());
    AUTO(mpool,
         MessagePool::create(
             {ipld, nullptr, nullptr, ts_load, interpreter_cache},
             nullptr,
             chain_store));

    void addMsgs(std::vector<SignedMessage> msgs, bool remove) {
      for (auto &msg : msgs) {
        mpool->add(msg).value();
      }
      if (remove) {
        for (auto &msg : msgs) {
          mpool->remove(msg.message.from, msg.message.nonce);
        }
      }
    }

    void setHead(TipsetCPtr ts) {
      chain_store->signal({{primitives::tipset::HeadChangeType::CURRENT, ts}});
    }
  };

  auto testMpoolSelectApply(Fixture &fix, double ticket_quality) {
    // will be removed by apply
    fix.addMsgs(msgs1, false);
    fix.setHead(ts2);
    cacheParentState(ts0);
    return fix.mpool->select(ts1, ticket_quality).value();
  }

  auto testMpoolSelectRevert(Fixture &fix, double ticket_quality) {
    // fill bls signature cache
    fix.addMsgs(msgs0, true);
    fix.setHead(ts0);
    cacheParentState(ts0);
    return fix.mpool->select(ts1, ticket_quality).value();
  }

  TEST(MpoolSelect, ApplyEmpty) {
    Fixture fix;
    EXPECT_TRUE(testMpoolSelectApply(fix, 0.5).empty());
  }

  TEST(MpoolSelect, Apply) {
    Fixture fix;
    fix.addMsgs(msgs0, false);
    EXPECT_FALSE(testMpoolSelectApply(fix, 0.5).empty());
  }

  struct MpoolSelectQualityTest : ::testing::TestWithParam<double> {};

  TEST_P(MpoolSelectQualityTest, Revert) {
    auto ticket_quality{GetParam()};
    Fixture fix;
    EXPECT_FALSE(testMpoolSelectRevert(fix, ticket_quality).empty());
  }

  INSTANTIATE_TEST_CASE_P(MpoolSelectQualityTest,
                          MpoolSelectQualityTest,
                          ::testing::Values(0.8, 0.9));
}  // namespace fc::storage::mpool
