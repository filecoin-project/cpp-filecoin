/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>

#include "fwd.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::storage::blockchain {
  using vm::runtime::MessageReceipt;

  class MsgWaiter : public std::enable_shared_from_this<MsgWaiter> {
   public:
    using Result = std::pair<MessageReceipt, TipsetKey>;
    using Callback = std::function<void(const Result &)>;

    static std::shared_ptr<MsgWaiter> create(
        TsLoadPtr ts_load,
        IpldPtr ipld,
        std::shared_ptr<ChainStore> chain_store);

    /**
     * Registers callback for the message by CID.
     *
     * @param cid - message CID
     * @param callback - action to call on message.
     */
    void wait(const CID &cid, const Callback &callback);

   private:
    /** Head change subscription. */
    outcome::result<void> onHeadChange(const HeadChange &change);

    TsLoadPtr ts_load;
    IpldPtr ipld;
    ChainStore::connection_t head_sub;
    mutable std::mutex mutex_waiting_;
    std::map<CID, std::vector<Callback>> waiting;
  };
}  // namespace fc::storage::blockchain
