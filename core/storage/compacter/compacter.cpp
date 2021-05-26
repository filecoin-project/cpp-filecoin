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
        start();
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
      _resume();
    }
  }

  bool CompacterIpld::start() {
    if (flag.exchange(true)) {
      return false;
    }
    thread.io->post([&] { _start(); });
    return true;
  }

  void CompacterIpld::_start() {
    spdlog::info("CompacterIpld._start");
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
      spdlog::error("CompacterIpld._start: create car failed: {:#}", ~car);
      return;
    }
    new_ipld = *car;
    new_ipld->io = old_ipld->io;
    new_ipld->flush_on = old_ipld->flush_on;
    queue->visited = new_ipld;
    queue->open(true);
    std::unique_lock vm_lock{*interpreter->mutex};
    std::shared_lock ts_lock{*ts_mutex};
    auto genesis{ts_load->lazyLoad(ts_main->chain.begin()->second).value()};
    start_head = ts_load->lazyLoad(ts_main->chain.rbegin()->second).value();
    ts_lock.unlock();
    queue->push(*asBlake(genesis->getParentStateRoot()));
    headers_top = genesis;
    _copy(*asBlake(genesis->key.cids()[0]));
    _pushState(*asBlake(start_head->getParentStateRoot()));
    state_bottom = start_head;
    headers_top_key.setCbor(headers_top->key.cids());
    start_head_key.setCbor(start_head->key.cids());
    {
      std::unique_lock ipld_lock{ipld_mutex};
      use_new_ipld = true;
    }
    vm_lock.unlock();
    thread.io->post([&] { _flow(); });
  }

  void CompacterIpld::_resume() {
    spdlog::info("CompacterIpld._resume");
    flag.store(true);
    std::unique_lock vm_lock{*interpreter->mutex};
    std::unique_lock ts_lock{*ts_mutex};
    auto car{cids_index::loadOrCreateWithProgress(
        path + ".car", true, old_ipld->max_memory, old_ipld->ipld, nullptr)};
    if (!car) {
      spdlog::error("CompacterIpld._resume: open car failed: {:#}", ~car);
      return;
    }
    new_ipld = *car;
    new_ipld->io = old_ipld->io;
    new_ipld->flush_on = old_ipld->flush_on;
    queue->visited = new_ipld;
    queue->open(false);
    start_head =
        ts_load->load(start_head_key.getCbor<std::vector<CID>>()).value();
    state_bottom = start_head;
    headers_top =
        ts_load->load(headers_top_key.getCbor<std::vector<CID>>()).value();
    {
      std::unique_lock ipld_lock{ipld_mutex};
      use_new_ipld = true;
    }
    thread.io->post([&] { _flow(); });
  }

  void CompacterIpld::_flow() {
    spdlog::info("CompacterIpld._flow");
    while (true) {
      std::shared_lock ts_lock{*ts_mutex};
      auto head1{ts_load->lazyLoad(ts_main->chain.rbegin()->second).value()};
      if (headers_top->key != head1->key) {
        _headersBatch();
      }
      auto done_headers{headers_top->key == head1->key};
      ts_lock.unlock();
      _queueLoop();
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
              _pushState(root);
            } else {
              _lookbackState(root);
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
    _finish();
  }

  void CompacterIpld::_headersBatch() {
    constexpr size_t kTsBatch{1000};
    auto it{ts_main->chain.find(headers_top->height())};
    while (it == ts_main->chain.end() || it->second.key != headers_top->key) {
      headers_top = ts_load->load(headers_top->getParents()).value();
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
        _copy(*asBlake(cid));
      }
      ++batch_used;
      ++it;
    }
    --it;
    headers_top = ts_load->lazyLoad(it->second).value();
    headers_top_key.setCbor(headers_top->key.cids());
  }

  void CompacterIpld::_queueLoop() {
    Buffer value;
    while (auto key{queue->pop()}) {
      if (!old_ipld->get(*key, value)) {
        spdlog::warn("CompacterIpld._queueLoop not found {}",
                     common::hex_lower(*key));
        continue;
      }
      queue->pushChildren(value);
      new_ipld->put(*key, value);
    }
  }

  void CompacterIpld::_finish() {
    spdlog::info("CompacterIpld._finish");
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
      }
    }
    auto head_res{interpreter_cache->get(head->key).value()};
    queue->push(*asBlake(head_res.state_root));
    queue->push(*asBlake(head_res.message_receipts));
    _queueLoop();
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

  void CompacterIpld::_pushState(const CbCid &state) {
    queue->push(state);
  }

  void CompacterIpld::_lookbackState(const CbCid &state) {
    std::vector<CbCid> copy, recurse;
    lookbackActors(copy, recurse, old_ipld, new_ipld, state);
    queue->push(recurse);
    for (auto it{copy.rbegin()}; it != copy.rend(); ++it) {
      _copy(*it);
    }
  }

  void CompacterIpld::_copy(const CbCid &key) {
    static Buffer value;
    if (old_ipld->get(key, value)) {
      new_ipld->put(key, value);
    } else if (!new_ipld->has(key)) {
      spdlog::warn("CompacterIpld._copy not found {}", common::hex_lower(key));
    }
  }
}  // namespace fc::storage::compacter
