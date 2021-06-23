/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/file.hpp"
#include "storage/car/cids_index/util.hpp"
#include "storage/compacter/util.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/storage/base_fs_test.hpp"

namespace fc::storage::compacter {
  struct CompacterTestMeta {
    std::vector<CbCid> ts_main;
    CID head_state, head_receipts;
    std::vector<std::vector<CbCid>> sync_branches;
  };
  CBOR_TUPLE(
      CompacterTestMeta, ts_main, head_state, head_receipts, sync_branches);

  struct CompacterTest : test::BaseFS_Test {
    std::string old_car, new_path;
    std::shared_ptr<CidsIpld> old_ipld;
    std::shared_ptr<CompacterIpld> compacter;
    std::shared_ptr<InMemoryStorage> compacter_kv;
    TipsetCPtr head;

    CompacterTest() : BaseFS_Test("cids_index_test") {}

    void SetUp() override {
      BaseFS_Test::SetUp();
      old_car = (getPathString() / "old.car").string();
      new_path = (getPathString() / "new").string();
      boost::filesystem::copy(resourcePath("compacter.car"), old_car);
      old_ipld = *cids_index::loadOrCreateWithProgress(
          old_car, true, boost::none, nullptr, nullptr);
      compacter_kv = std::make_shared<InMemoryStorage>();
      _init();
    }

    void _unref() {
      if (compacter) {
        compacter->ts_load.reset();
        compacter->interpreter_cache.reset();
      }
    }

    void TearDown() override {
      _unref();
      BaseFS_Test::TearDown();
    }

    void _init() {
      _unref();
      compacter = make(new_path,
                       compacter_kv,
                       old_ipld,
                       std::make_shared<std::shared_mutex>());

      auto meta{
          old_ipld
              ->getCbor<CompacterTestMeta>(car::readHeader(old_car).value()[0])
              .value()};
      auto ts_load{std::make_shared<primitives::tipset::TsLoadIpld>(
          std::make_shared<CbAsAnyIpld>(compacter))};
      head = ts_load->load(meta.ts_main).value();
      auto ts_main{primitives::tipset::chain::file::loadOrCreate(
          nullptr,
          (getPathString() / "ts-chain").string(),
          compacter,
          head->key.cids(),
          0)};
      EXPECT_TRUE(ts_main);
      auto ts_branches{std::make_shared<TsBranches>()};
      ts_branches->insert(ts_main);
      for (auto &cids : meta.sync_branches) {
        primitives::tipset::chain::TsChain chain;
        TipsetKey tsk{cids};
        while (auto _ts{ts_load->load(tsk)}) {
          auto &ts{_ts.value()};
          chain.emplace(ts->height(), primitives::tipset::TsLazy{tsk});
          tsk = ts->getParents();
        }
        ts_branches->insert(TsBranch::make(std::move(chain)));
      }
      auto interpreter_cache{std::make_shared<InterpreterCache>(
          std::make_shared<InMemoryStorage>(), compacter)};
      interpreter_cache->set(head->key,
                             {meta.head_state, meta.head_receipts, {}});

      compacter->interpreter_cache = interpreter_cache;
      compacter->ts_load = ts_load;
      compacter->ts_main = ts_main;
      compacter->ts_branches = ts_branches;

      compacter->thread.work.reset();
      compacter->thread.thread.join();
      compacter->thread.io = std::make_shared<boost::asio::io_context>();
    }

    void runOne() {
      compacter->thread.io->run_one_for(std::chrono::seconds{10});
    }
  };

  TEST_F(CompacterTest, Flow) {
    compacter->open();
    compacter->compact_on_car = old_ipld->car_offset - 1;
    compacter->epochs_full_state = 1;
    compacter->epochs_lookback_state = compacter->epochs_full_state + 1;
    compacter->epochs_messages = 1;
    std::ofstream{new_path + ".car"};
    std::ofstream{new_path + ".car.cids"};
    compacter->put(codec::cbor::encode("test").value());
    EXPECT_FALSE(compacter->asyncStart());
    runOne();
    runOne();
  }

  TEST_F(CompacterTest, Resume) {
    compacter->thread.io = std::make_shared<boost::asio::io_context>();
    compacter->open();
    EXPECT_TRUE(compacter->asyncStart());
    runOne();
    _init();
    compacter->open();
    EXPECT_FALSE(compacter->asyncStart());
    runOne();
  }
}  // namespace fc::storage::compacter
