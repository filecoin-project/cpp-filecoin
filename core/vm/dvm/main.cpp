/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cbor_blake/ipld_any.hpp"
#include "primitives/tipset/chain.hpp"
#include "storage/car/car.hpp"
#include "storage/car/cids_index/util.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "vm/actor/cgo/actors.hpp"
#include "vm/actor/impl/invoker_impl.hpp"
#include "vm/dvm/dvm.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"
#include "vm/runtime/circulating.hpp"
#include "vm/runtime/impl/tipset_randomness.hpp"

int main(int argc, char **argv) {
  using namespace fc;
  using primitives::tipset::Height;

  vm::actor::cgo::configParams();
  // mainnet genesis
  auto genesis_cid{
      CID::fromString(
          "bafy2bzacecnamqgqmifpluoeldx7zzglxcljo6oja4vrmtj7432rphldpdmm2")
          .value()};

  if (argc > 1) {
    std::string car_path{argv[1]};
    auto ipld_mem{std::make_shared<storage::ipfs::InMemoryDatastore>()};
    // TODO(turuslan): max memory
    if (auto _ipld{storage::cids_index::loadOrCreateWithProgress(
            car_path, false, boost::none, ipld_mem, nullptr)}) {
      vm::runtime::EnvironmentContext envx;
      envx.ipld = _ipld.value();
      envx.ts_branches_mutex = std::make_shared<std::shared_mutex>();
      envx.invoker = std::make_shared<vm::actor::InvokerImpl>();
      auto ts_load_ipld{
          std::make_shared<fc::primitives::tipset::TsLoadIpld>(envx.ipld)};
      envx.ts_load =
          std::make_shared<primitives::tipset::TsLoadCache>(ts_load_ipld, 1000);
      envx.randomness = std::make_shared<vm::runtime::TipsetRandomness>(
          envx.ts_load, envx.ts_branches_mutex);
      envx.interpreter_cache =
          std::make_shared<vm::interpreter::InterpreterCache>(
              std::make_shared<storage::InMemoryStorage>(),
              std::make_shared<AnyAsCbIpld>(envx.ipld));
      envx.circulating = vm::Circulating::make(envx.ipld, genesis_cid).value();
      vm::interpreter::InterpreterImpl vmi{envx, nullptr};

      const auto head_tsk{
          *TipsetKey::make(storage::car::readHeader(car_path).value())};
      auto head{envx.ts_load->loadWithCacheInfo(head_tsk).value()};
      primitives::tipset::chain::TsChain _chain;
      Height state_min_height{head.tipset->height()}, state_max_height{0};
      auto ts_lookback{4000};
      auto had_states{true};
      auto ts{head};
      while (true) {
        _chain.emplace(ts.tipset->height(),
                       primitives::tipset::TsLazy{ts.tipset->key, ts.index});
        if (envx.ipld->contains(ts.tipset->getParentStateRoot()).value()) {
          if (had_states) {
            state_min_height = std::min(state_min_height, ts.tipset->height());
            state_max_height = std::max(state_max_height, ts.tipset->height());
          }
        } else {
          had_states = false;
        }
        if (ts.tipset->height() + ts_lookback < state_min_height) {
          break;
        }
        if (auto _ts{
                envx.ts_load->loadWithCacheInfo(ts.tipset->getParents())}) {
          ts = _ts.value();
        } else {
          break;
        }
      }
      auto branch{TsBranch::make(std::move(_chain))};

      if (state_min_height > state_max_height) {
        spdlog::error("no ts with states");
        exit(EXIT_FAILURE);
      } else {
        spdlog::info(
            "ts with states: {}..{}", state_min_height, state_max_height);
      }
      if (argc > 2) {
        auto min_height{
            std::max<Height>(state_min_height, std::stoull(argv[2]))};
        auto max_height{min_height};
        if (argc > 3) {
          max_height =
              std::min(state_max_height,
                       std::max<Height>(min_height, std::stoull(argv[3])));
        }
        if (dvm::logger) {
          dvm::logging = true;
        }
        for (auto it{branch->chain.lower_bound(min_height)};
             it != branch->chain.end() && it->first <= max_height;
             ++it) {
          auto parent{envx.ts_load->lazyLoad(it->second).value()};
          primitives::tipset::TipsetCPtr child;
          if (auto _child{std::next(it)}; _child != branch->chain.end()) {
            child = envx.ts_load->lazyLoad(_child->second).value();
          }
          spdlog::info("height {}", parent->height());
          if (auto _res{vmi.interpret(branch, parent)}) {
            auto &res{_res.value()};
            if (child) {
              if (res.state_root != child->getParentStateRoot()) {
                spdlog::error("state differs");
                exit(EXIT_FAILURE);
              }
              if (res.message_receipts != child->getParentMessageReceipts()) {
                spdlog::error("receipts differ");
                exit(EXIT_FAILURE);
              }
            }
          } else {
            spdlog::error("interpret {:#}", _res.error());
            exit(EXIT_FAILURE);
          }
          spdlog::info("ok");
        }
        spdlog::info("done");
      }
    }
  } else {
    fmt::print("usage: {} CAR [MIN_HEIGHT [MAX_HEIGHT]]\n", argv[0]);
  }
}
