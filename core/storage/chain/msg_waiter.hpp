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

  // TODO(turuslan): "allow_replaced" param to ignore message gas
  class MsgWaiter : public std::enable_shared_from_this<MsgWaiter> {
   public:
    using Callback = std::function<void(TipsetCPtr, MessageReceipt)>;
    struct Wait {
      // TODO(turuslan): FIL-420 check cache memory usage
      std::multimap<ChainEpoch, Callback> callbacks;
      TipsetCPtr ts;
      MessageReceipt receipt;
    };
    using Waiting = std::map<CID, Wait>;
    struct Search {
      CID cid;
      Callback cb;
      ChainEpoch min_height{};
      TipsetCPtr ts;
    };
    using Searching = std::list<Search>;

    static std::shared_ptr<MsgWaiter> create(
        TsLoadPtr ts_load,
        IpldPtr ipld,
        std::shared_ptr<boost::asio::io_context> io,
        const std::shared_ptr<ChainStore>& chain_store);

    void search(TipsetCPtr ts,
                const CID &cid,
                ChainEpoch lookback_limit,
                Callback cb);
    void wait(const CID &cid,
              ChainEpoch lookback_limit,
              EpochDuration confidence,
              Callback cb);

   private:
    /** Head change subscription. */
    outcome::result<void> onHeadChange(const HeadChange &change);
    outcome::result<bool> isSearch(const CID &cid);
    void _search(TipsetCPtr ts,
                 const CID &cid,
                 ChainEpoch lookback_limit,
                 Callback cb);
    void searchLoop(Searching::iterator it);
    Waiting::iterator checkWait(Waiting::iterator it);

    TsLoadPtr ts_load;
    IpldPtr ipld;
    std::shared_ptr<boost::asio::io_context> io;
    ChainStore::connection_t head_sub;
    mutable std::mutex mutex;
    TipsetCPtr head;
    std::shared_ptr<vm::state::StateTreeImpl> state_tree;
    // TODO(turuslan): FIL-420 check cache memory usage
    Waiting waiting;
    // TODO(turuslan): FIL-420 check cache memory usage
    Searching searching;
  };
}  // namespace fc::storage::blockchain
