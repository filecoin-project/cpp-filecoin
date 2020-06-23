/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_MSG_WAITER_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_MSG_WAITER_HPP

#include "storage/chain/chain_store.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::storage::blockchain {
  using vm::runtime::MessageReceipt;

  struct MsgWaiter : public std::enable_shared_from_this<MsgWaiter> {
    using Result = std::pair<MessageReceipt, TipsetKey>;
    using Callback = std::function<void(const Result &)>;

    explicit MsgWaiter(IpldPtr ipld);
    static std::shared_ptr<MsgWaiter> create(
        IpldPtr ipld, std::shared_ptr<ChainStore> chain_store);
    outcome::result<void> onHeadChange(const HeadChange &change);
    void wait(const CID &cid, const Callback &callback);

    IpldPtr ipld;
    ChainStore::connection_t head_sub;
    std::map<CID, Result> results;
    std::map<CID, std::vector<Callback>> waiting;
  };
}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_MSG_WAITER_HPP
