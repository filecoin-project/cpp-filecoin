/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/msg_waiter.hpp"
#include "adt/array.hpp"

namespace fc::storage::blockchain {
  using primitives::tipset::MessageVisitor;

  MsgWaiter::MsgWaiter(IpldPtr ipld) : ipld{ipld} {}

  std::shared_ptr<MsgWaiter> MsgWaiter::create(
      IpldPtr ipld, std::shared_ptr<ChainStore> chain_store) {
    auto waiter{std::make_shared<MsgWaiter>(ipld)};
    waiter->head_sub = chain_store->subscribeHeadChanges([=](auto &change) {
      auto res{waiter->onHeadChange(change)};
      if (!res) {
        spdlog::error("MsgWaiter.onHeadChange: error {} \"{}\"",
                      res.error(),
                      res.error().message());
      }
    });
    return waiter;
  }

  outcome::result<void> MsgWaiter::onHeadChange(const HeadChange &change) {
    auto onTipset = [&](auto &ts, auto apply) -> outcome::result<TipsetCPtr> {
      OUTCOME_TRY(parent, ts->loadParent(*ipld));
      adt::Array<MessageReceipt> receipts{ts->getParentMessageReceipts(), ipld};
      OUTCOME_TRY(parent->visitMessages(
          ipld, [&](auto i, auto, auto &cid) -> outcome::result<void> {
            if (apply) {
              OUTCOME_TRY(receipt, receipts.get(i));
              auto result{
                  results.emplace(cid, std::make_pair(receipt, ts->key))};
              auto callbacks{waiting.find(cid)};
              if (callbacks != waiting.end()) {
                for (auto &callback : callbacks->second) {
                  callback(result.first->second);
                }
                waiting.erase(cid);
              }
            } else {
              results.erase(cid);
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
    auto result{results.find(cid)};
    if (result != results.end()) {
      callback(result->second);
    } else {
      waiting[cid].push_back(callback);
    }
  }
}  // namespace fc::storage::blockchain
