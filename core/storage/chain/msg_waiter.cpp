/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/msg_waiter.hpp"

#include <boost/asio/io_context.hpp>
#include <utility>

#include "adt/array.hpp"
#include "cbor_blake/ipld_version.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "primitives/tipset/load.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

namespace fc::storage::blockchain {
  using primitives::tipset::MessageVisitor;

  std::shared_ptr<MsgWaiter> MsgWaiter::create(
      TsLoadPtr ts_load,
      IpldPtr ipld,
      std::shared_ptr<boost::asio::io_context> io,
      const std::shared_ptr<ChainStore> &chain_store) {
    auto waiter{std::make_shared<MsgWaiter>()};
    waiter->ts_load = std::move(ts_load);
    waiter->ipld = std::move(ipld);
    waiter->io = std::move(io);
    waiter->head_sub =
        chain_store->subscribeHeadChanges([=](const auto &changes) {
          for (const auto &change : changes) {
            auto res{waiter->onHeadChange(change)};
            if (!res) {
              spdlog::error("MsgWaiter.onHeadChange: {:#}", res.error());
            }
          }
        });
    return waiter;
  }

  void MsgWaiter::search(TipsetCPtr ts,
                         const CID &cid,
                         ChainEpoch lookback_limit,
                         Callback cb) {
    std::unique_lock lock{mutex};
    const auto is_search{isSearch(cid)};
    if (!is_search || !is_search.value()) {
      return cb({}, {});
    }
    _search(std::move(ts), cid, lookback_limit, std::move(cb));
  }

  void MsgWaiter::wait(const CID &cid,
                       ChainEpoch lookback_limit,
                       EpochDuration confidence,
                       Callback cb) {
    std::unique_lock lock{mutex};
    const auto is_search{isSearch(cid)};
    if (!is_search) {
      return cb({}, {});
    }
    if (is_search.value()) {
      _search(head,
              cid,
              lookback_limit,
              [=, cb{std::move(cb)}](auto ts, auto receipt) mutable {
                if (!ts) {
                  return cb({}, {});
                }
                const auto revert{head->height() < ts->height()};
                if (!revert && head->height() - ts->height() >= confidence) {
                  return cb(std::move(ts), std::move(receipt));
                }
                auto it{waiting.find(cid)};
                if (it == waiting.end()) {
                  it = waiting.emplace(cid, Wait{}).first;
                }
                auto &wait{it->second};
                wait.callbacks.emplace(confidence, std::move(cb));
                if (!wait.ts && !revert) {
                  wait.ts = std::move(ts);
                  wait.receipt = std::move(receipt);
                }
                checkWait(it);
              });
      return;
    }
    waiting[cid].callbacks.emplace(confidence, std::move(cb));
  }

  // NOLIINTNEXTLINE(readability-function-cognitive-complexity)
  outcome::result<void> MsgWaiter::onHeadChange(const HeadChange &change) {
    std::unique_lock lock{mutex};
    const auto &ts{change.value};
    if (change.type == HeadChangeType::CURRENT) {
      head = ts;
    } else {
      const auto apply{change.type == HeadChangeType::APPLY};
      OUTCOME_TRY(parent, ts_load->load(ts->getParents()));
      head = apply ? ts : parent;
      adt::Array<MessageReceipt> receipts{ts->getParentMessageReceipts(), ipld};
      OUTCOME_TRY(parent->visitMessages(
          {ipld, true, false},
          [&](auto i, auto, auto &cid, auto, auto) -> outcome::result<void> {
            auto it{waiting.find(cid)};
            if (apply) {
              if (it != waiting.end()) {
                OUTCOME_TRYA(it->second.receipt, receipts.get(i));
                it->second.ts = ts;
              }
            } else {
              if (it != waiting.end()) {
                if (it->second.ts->key == ts->key) {
                  it->second.ts.reset();
                }
              }
            }
            return outcome::success();
          }));
      for (auto it{waiting.begin()}; it != waiting.end();) {
        it = checkWait(it);
      }
    }
    state_tree = std::make_shared<vm::state::StateTreeImpl>(
        withVersion(ipld, head->height()), head->getParentStateRoot());
    return outcome::success();
  }

  outcome::result<bool> MsgWaiter::isSearch(const CID &cid) {
    OUTCOME_TRY(cbor, ipld->get(cid));
    OUTCOME_TRY(msg, vm::message::UnsignedMessage::decode(cbor));
    OUTCOME_TRY(actor, state_tree->get(msg.from));
    return msg.nonce < actor.nonce;
  }

  void MsgWaiter::_search(TipsetCPtr ts,
                          const CID &cid,
                          ChainEpoch lookback_limit,
                          Callback cb) {
    Search search;
    search.cid = cid;
    search.cb = std::move(cb);
    search.min_height =
        lookback_limit == -1 ? 0 : head->epoch() - lookback_limit;
    search.ts = std::move(ts);
    searchLoop(searching.emplace(searching.end(), std::move(search)));
  }

  void MsgWaiter::searchLoop(Searching::iterator it) {
    auto &search{*it};
    TipsetCPtr ts_found;
    MessageReceipt receipt;
    if (search.ts->epoch() != 0 && search.ts->epoch() >= search.min_height) {
      if (auto _ts{ts_load->load(search.ts->getParents())}) {
        auto &parent{_ts.value()};
        adt::Array<MessageReceipt> receipts{
            search.ts->getParentMessageReceipts(), ipld};
        auto visit{parent->visitMessages(
            {ipld, true, false},
            [&](auto i, auto, auto &cid, auto, auto) -> outcome::result<void> {
              if (cid == search.cid) {
                OUTCOME_TRYA(receipt, receipts.get(i));
                ts_found = search.ts;
              }
              return outcome::success();
            })};
        if (visit && !ts_found) {
          search.ts = std::move(parent);
          io->post([=, weak{weak_from_this()}] {
            if (auto self{weak.lock()}) {
              std::unique_lock lock{mutex};
              searchLoop(it);
            }
          });
          return;
        }
      }
    }
    search.cb(std::move(ts_found), std::move(receipt));
    searching.erase(it);
  }

  MsgWaiter::Waiting::iterator MsgWaiter::checkWait(Waiting::iterator it) {
    auto &wait{it->second};
    if (wait.ts) {
      const auto confidence{head->height() - wait.ts->height()};
      for (auto it2{wait.callbacks.begin()}; it2 != wait.callbacks.end();) {
        if (confidence >= it2->first) {
          io->post(std::bind(std::move(it2->second), wait.ts, wait.receipt));
          it2 = wait.callbacks.erase(it2);
        } else {
          ++it2;
        }
      }
      if (wait.callbacks.empty()) {
        return waiting.erase(it);
      }
    }
    return std::next(it);
  }
}  // namespace fc::storage::blockchain
