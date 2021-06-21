/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/compacter/compacter.hpp"

#include "storage/car/cids_index/util.hpp"
#include "storage/compacter/lookback.hpp"

namespace fc::vm::interpreter {
  outcome::result<Result> CompacterInterpreter::interpret(
      TsBranchPtr ts_branch, const TipsetCPtr &tipset) const {
    std::shared_lock lock{*mutex};
    return interpreter->interpret(ts_branch, tipset);
  }
}  // namespace fc::vm::interpreter

namespace fc::storage::compacter {
  void CompacterPutBlockHeader::put(const CbCid &key, BytesIn value) {
    if (auto compacter{_compacter.lock()}) {
      std::shared_lock lock{compacter->ipld_mutex};
      (compacter->use_new_ipld ? compacter->new_ipld : compacter->old_ipld)
          ->put(key, value);
    }
  }

  bool CompacterIpld::get(const CbCid &key, Buffer *value) const {
    std::shared_lock lock{ipld_mutex};
    if (use_new_ipld && new_ipld->get(key, value)) {
      return true;
    }
    return old_ipld->get(key, value);
  }

  void CompacterIpld::put(const CbCid &key, BytesIn value) {
    std::shared_lock lock{ipld_mutex};
    if (compact_on_car && !flag.load()) {
      std::shared_lock written_lock{old_ipld->written_mutex};
      if (old_ipld->car_offset > compact_on_car) {
        asyncStart();
      }
    }
    if (use_new_ipld) {
      queue->pushChildren(value);
      new_ipld->put(key, value);
    } else {
      old_ipld->put(key, value);
    }
  }

  void CompacterIpld::open() {
    if (start_head_key.has()) {
      resume();
    }
  }

  bool CompacterIpld::asyncStart() {
    if (flag.exchange(true)) {
      return false;
    }
    thread.io->post([&] { doStart(); });
    return true;
  }

  void CompacterIpld::doStart() {
    spdlog::info("CompacterIpld.doStart");
    auto car_path{path + ".car"};
    if (boost::filesystem::exists(car_path)) {
      boost::filesystem::remove(car_path);
    }
    auto index_path{car_path + ".cids"};
    if (boost::filesystem::exists(index_path)) {
      boost::filesystem::remove(index_path);
    }
    auto car{cids_index::loadOrCreateWithProgress(
        car_path, true, old_ipld->max_memory, old_ipld->ipld, nullptr)};
    if (!car) {
      spdlog::error("CompacterIpld.doStart: create car failed: {:#}", ~car);
      return;
    }
    new_ipld = *car;
    new_ipld->io = old_ipld->io;
    new_ipld->flush_on = old_ipld->flush_on;
    queue->visited = new_ipld;
    queue->open(true);
    std::unique_lock vm_lock{*interpreter->mutex};
    std::shared_lock ts_lock{*ts_mutex};
    auto genesis{ts_load->lazyLoad(ts_main->bottom().second).value()};
    start_head = ts_load->lazyLoad(ts_main->chain.rbegin()->second).value();
    ts_lock.unlock();
    queue->push(*asBlake(genesis->getParentStateRoot()));
    headers_top = genesis;
    copy(genesis->key.cids()[0]);
    pushState(*asBlake(start_head->getParentStateRoot()));
    state_bottom = start_head;
    headers_top_key.setCbor(headers_top->key.cids());
    start_head_key.setCbor(start_head->key.cids());
    {
      std::unique_lock ipld_lock{ipld_mutex};
      use_new_ipld = true;
    }
    vm_lock.unlock();
    thread.io->post([&] { flow(); });
  }

  void CompacterIpld::resume() {
    spdlog::info("CompacterIpld.resume");
    flag.store(true);
    std::unique_lock vm_lock{*interpreter->mutex};
    std::unique_lock ts_lock{*ts_mutex};
    auto car{cids_index::loadOrCreateWithProgress(
        path + ".car", true, old_ipld->max_memory, old_ipld->ipld, nullptr)};
    if (!car) {
      spdlog::error("CompacterIpld.resume: open car failed: {:#}", ~car);
      return;
    }
    new_ipld = *car;
    new_ipld->io = old_ipld->io;
    new_ipld->flush_on = old_ipld->flush_on;
    queue->visited = new_ipld;
    queue->open(false);
    start_head =
        ts_load->load(start_head_key.getCbor<std::vector<CbCid>>()).value();
    state_bottom = start_head;
    headers_top =
        ts_load->load(headers_top_key.getCbor<std::vector<CbCid>>()).value();
    {
      std::unique_lock ipld_lock{ipld_mutex};
      use_new_ipld = true;
    }
    thread.io->post([&] { flow(); });
  }

  void CompacterIpld::flow() {
    spdlog::info("CompacterIpld.flow");
    while (true) {
      std::shared_lock ts_lock{*ts_mutex};
      auto head1{ts_load->lazyLoad(ts_main->chain.rbegin()->second).value()};
      if (headers_top->key != head1->key) {
        headersBatch();
      }
      auto done_headers{headers_top->key == head1->key};
      ts_lock.unlock();
      queueLoop();
      ts_lock.lock();
      auto head2{ts_load->lazyLoad(ts_main->chain.rbegin()->second).value()};
      ts_lock.unlock();
      auto done_state{!state_bottom || state_bottom->height() == 0};
      if (!done_state) {
        auto epochs{head2->height() - state_bottom->height()};
        epochs_lookback_state =
            std::max(epochs_lookback_state, epochs_full_state);
        if (epochs <= epochs_lookback_state) {
          state_bottom = ts_load->load(state_bottom->getParents()).value();
          auto root{*asBlake(state_bottom->getParentStateRoot())};
          if (state_bottom->height() == 0 || !old_ipld->has(root)) {
            done_state = true;
            state_bottom.reset();
          } else {
            if (epochs <= epochs_full_state) {
              pushState(root);
            } else {
              lookbackState(root);
            }
          }
        } else {
          done_state = true;
        }
      }
      if (done_headers && done_state) {
        break;
      }
    }
    finish();
  }

  void CompacterIpld::headersBatch() {
    constexpr size_t kTsBatch{1000};
    ts_main->lazyLoad(headers_top->height());
    auto it{ts_main->chain.find(headers_top->height())};
    while (it == ts_main->chain.end() || it->second.key != headers_top->key) {
      headers_top = ts_load->load(headers_top->getParents()).value();
      ts_main->lazyLoad(headers_top->height());
      it = ts_main->chain.find(headers_top->height());
    }
    ++it;
    size_t batch_used{};
    while (true) {
      if (it == ts_main->chain.end()) {
        break;
      }
      if (batch_used >= kTsBatch) {
        break;
      }
      for (auto &cid : it->second.key.cids()) {
        copy(cid);
      }
      ++batch_used;
      ++it;
    }
    --it;
    headers_top = ts_load->lazyLoad(it->second).value();
    headers_top_key.setCbor(headers_top->key.cids());
  }

  void CompacterIpld::queueLoop() {
    auto &value{reuse_buffer};
    while (auto key{queue->pop()}) {
      if (!old_ipld->get(*key, value)) {
        spdlog::warn("CompacterIpld.queueLoop not found {}",
                     common::hex_lower(*key));
        continue;
      }
      queue->pushChildren(value);
      new_ipld->put(*key, value);
    }
  }

  void CompacterIpld::finish() {
    spdlog::info("CompacterIpld.finish");
    std::unique_lock vm_lock{*interpreter->mutex};
    std::unique_lock ts_lock{*ts_mutex};
    auto head{ts_load->lazyLoad(ts_main->chain.rbegin()->second).value()};
    auto ts{head};
    for (size_t i{}; i < epochs_messages; ++i) {
      for (auto &block : ts->blks) {
        queue->push(*asBlake(block.messages));
      }
      auto receipt{*asBlake(ts->getParentMessageReceipts())};
      if (old_ipld->has(receipt)) {
        queue->push(receipt);
      }
      if (ts->height() == 0) {
        break;
      }
      ts = ts_load->load(ts->getParents()).value();
    }
    const auto head_res{interpreter_cache->get(head->key).value()};
    queue->push(*asBlake(head_res.state_root));
    queue->push(*asBlake(head_res.message_receipts));
    for (auto &branch : *ts_branches) {
      if (branch == ts_main) {
        continue;
      }
      for (auto it : branch->chain) {
        auto ts{ts_load->lazyLoad(it.second).value()};
        for (auto &block : ts->blks) {
          primitives::tipset::put(nullptr, put_block_header, block);
          queue->push(*asBlake(block.messages));
        }
        // keep interpreted tipsets that can be attached to the main branch
        if (ts->height() >= head->height()) {
          if (auto res{interpreter_cache->tryGet(ts->key)}; res && *res) {
            queue->push(*asBlake(res->value().state_root));
            queue->push(*asBlake(res->value().message_receipts));
          }
        }
      }
    }
    queueLoop();
    queue->clear();
    {
      std::unique_lock old_flush_lock{old_ipld->flush_mutex};
      std::unique_lock new_flush_lock{new_ipld->flush_mutex};
      std::unique_lock ipld_lock{ipld_mutex};
      // keep last car copy for debug
      boost::filesystem::rename(old_ipld->car_path,
                                old_ipld->car_path + ".old_ipld");
      boost::filesystem::rename(new_ipld->car_path, old_ipld->car_path);
      boost::filesystem::rename(new_ipld->index_path, old_ipld->index_path);
      new_ipld->car_path = old_ipld->car_path;
      new_ipld->index_path = old_ipld->index_path;
      use_new_ipld = false;
      old_ipld = new_ipld;
      new_ipld.reset();
    }
    start_head_key.remove();
    flag.store(false);
    spdlog::info("CompacterIpld done");
  }

  void CompacterIpld::pushState(const CbCid &state) {
    queue->push(state);
  }

  void CompacterIpld::lookbackState(const CbCid &state) {
    std::vector<CbCid> copy, recurse;
    lookbackActors(copy, recurse, old_ipld, new_ipld, state);
    queue->push(recurse);
    for (auto it{copy.rbegin()}; it != copy.rend(); ++it) {
      this->copy(*it);
    }
  }

  void CompacterIpld::copy(const CbCid &key) {
    auto &value{reuse_buffer};
    if (old_ipld->get(key, value)) {
      new_ipld->put(key, value);
    } else if (!new_ipld->has(key)) {
      spdlog::warn("CompacterIpld.copy not found {}", common::hex_lower(key));
    }
  }
}  // namespace fc::storage::compacter
