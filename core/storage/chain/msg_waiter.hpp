/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "fwd.hpp"
#include "storage/chain/chain_store.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::storage::blockchain {
  using vm::runtime::MessageReceipt;

  struct MsgWaiter : public std::enable_shared_from_this<MsgWaiter> {
    using Result = std::pair<MessageReceipt, TipsetKey>;
    using Callback = std::function<void(const Result &)>;

    static std::shared_ptr<MsgWaiter> create(
        TsLoadPtr ts_load,
        IpldPtr ipld,
        std::shared_ptr<ChainStore> chain_store);
    outcome::result<void> onHeadChange(const HeadChange &change);
    void wait(const CID &cid, const Callback &callback);

    TsLoadPtr ts_load;
    IpldPtr ipld;
    ChainStore::connection_t head_sub;
    std::map<CID, Result> results;
    std::map<CID, std::vector<Callback>> waiting;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::storage::blockchain::MsgWaiter);
  };
}  // namespace fc::storage::blockchain
