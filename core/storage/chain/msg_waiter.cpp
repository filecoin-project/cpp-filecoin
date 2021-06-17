/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/msg_waiter.hpp"
#include "adt/array.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "primitives/tipset/load.hpp"

namespace fc::storage::blockchain {
  using primitives::tipset::MessageVisitor;

  std::shared_ptr<MsgWaiter> MsgWaiter::create(
      TsLoadPtr ts_load,
      IpldPtr ipld,
      std::shared_ptr<ChainStore> chain_store) {
    auto waiter{std::make_shared<MsgWaiter>()};
    waiter->ts_load = std::move(ts_load);
    waiter->ipld = std::move(ipld);
    waiter->head_sub = chain_store->subscribeHeadChanges([=](auto &change) {
      auto res{waiter->onHeadChange(change)};
      if (!res) {
        spdlog::error("MsgWaiter.onHeadChange: {:#}", res.error());
      }
    });
    return waiter;
  }

  outcome::result<void> MsgWaiter::onHeadChange(const HeadChange &change) {
    auto onTipset = [&](auto &ts, auto apply) -> outcome::result<TipsetCPtr> {
      OUTCOME_TRY(parent, ts_load->load(ts->getParents()));
      adt::Array<MessageReceipt> receipts{ts->getParentMessageReceipts(), ipld};
      OUTCOME_TRY(parent->visitMessages(
          {ipld, true, false},
          [&](auto i, auto, auto &cid, auto, auto) -> outcome::result<void> {
            if (apply) {
              std::lock_guard lock{mutex_waiting_};
              auto callbacks{waiting.find(cid)};
              if (callbacks != waiting.end()) {
                OUTCOME_TRY(receipt, receipts.get(i));
                for (auto &callback : callbacks->second) {
                  callback({receipt, ts->key});
                }
                waiting.erase(cid);
              }
            }
            return outcome::success();
          }));
      return std::move(parent);
    };
    if (change.type == HeadChangeType::CURRENT) {
      auto ts{change.value};
      while (ts->height() > 0) {
        OUTCOME_TRYA(ts, onTipset(ts, true));
      }
    } else {
      OUTCOME_TRY(onTipset(change.value, change.type == HeadChangeType::APPLY));
    }
    return outcome::success();
  }

  void MsgWaiter::wait(const CID &cid, const Callback &callback) {
    std::lock_guard lock{mutex_waiting_};
    waiting[cid].push_back(callback);
  }
}  // namespace fc::storage::blockchain
