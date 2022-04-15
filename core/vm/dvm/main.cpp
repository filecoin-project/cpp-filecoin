/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/predicate.hpp>

#include "cbor_blake/ipld_any.hpp"
#include "cbor_blake/memory.hpp"
#include "drand/impl/beaconizer.hpp"
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

namespace fc {
  using storage::cids_index::CidsIpld;

  struct Magic : CbIpld {
    std::vector<std::shared_ptr<CidsIpld>> cars;
    MemoryCbIpld cache;

    bool get(const CbCid &key, Bytes *value) const override {
      if (cache.get(key, value)) {
        return true;
      }
      for (auto &car : cars) {
        if (car->get(key, value)) {
          return true;
        }
      }
      return false;
    }
    void put(const CbCid &key, BytesCow &&value) override {
      cache.put(key, std::move(value));
    }
  };
}  // namespace fc

int main(int argc, char **argv) {
  using namespace fc;
  using primitives::ChainEpoch;

  vm::actor::cgo::configParams();
  // mainnet genesis
  auto genesis_cid{
      CID::fromString(
          "bafy2bzacecnamqgqmifpluoeldx7zzglxcljo6oja4vrmtj7432rphldpdmm2")
          .value()};

  auto ipld{std::make_shared<Magic>()};
  std::optional<CbCid> child_cid;
  for (auto i{1}; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (boost::ends_with(arg, ".car")) {
      ipld->cars.push_back(*storage::cids_index::loadOrCreateWithProgress(
          std::string{arg}, false, {}, {}, {}));
    } else {
      if (child_cid) throw "many children";
      child_cid.emplace(CbCid{CbCid::fromHex(arg).value()});
    }
  }
  if (!ipld) throw "no cars";
  if (!child_cid) throw "no child";

  {
    {
      vm::runtime::EnvironmentContext envx;
      envx.ipld = std::make_shared<CbAsAnyIpld>(ipld);
      envx.ts_branches_mutex = std::make_shared<std::shared_mutex>();
      envx.invoker = std::make_shared<vm::actor::InvokerImpl>();
      auto ts_load_ipld{
          std::make_shared<fc::primitives::tipset::TsLoadIpld>(envx.ipld)};
      envx.ts_load =
          std::make_shared<primitives::tipset::TsLoadCache>(ts_load_ipld, 1000);

      const auto genesis_block{
          getCbor<primitives::block::BlockHeader>(envx.ipld, genesis_cid)
              .value()};
      envx.randomness = std::make_shared<vm::runtime::TipsetRandomness>(
          envx.ts_load,
          envx.ts_branches_mutex,
          std::make_shared<drand::DrandScheduleImpl>(
              drand::ChainInfo{
                  {},
                  std::chrono::seconds{1595431050},
                  std::chrono::seconds{30},
              },
              std::chrono::seconds{genesis_block.timestamp},
              std::chrono::seconds{kBlockDelaySecs}));
      envx.interpreter_cache =
          std::make_shared<vm::interpreter::InterpreterCache>(
              std::make_shared<storage::InMemoryStorage>(),
              std::make_shared<AnyAsCbIpld>(envx.ipld));
      envx.circulating = vm::Circulating::make(envx.ipld, genesis_cid).value();
      vm::interpreter::InterpreterImpl vmi{envx, nullptr, nullptr};

      const TipsetKey head_tsk{{*child_cid}};
      auto head{envx.ts_load->loadWithCacheInfo(head_tsk).value()};
      primitives::tipset::chain::TsChain _chain;
      auto ts_lookback{4000};
      auto ts{head};
      for (auto i{0}; i < ts_lookback; ++i) {
        _chain.emplace(ts.tipset->height(),
                       primitives::tipset::TsLazy{ts.tipset->key, ts.index});
        if (auto _ts{
                envx.ts_load->loadWithCacheInfo(ts.tipset->getParents())}) {
          ts = _ts.value();
        } else {
          break;
        }
      }
      auto branch{TsBranch::make(std::move(_chain))};

      {
        if (dvm::logger) {
          dvm::logging = true;
        }
        {
          auto child{head.tipset};
          auto parent{envx.ts_load->load(child->getParents()).value()};
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
            } else {
              spdlog::warn("no child");
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
  }
}
