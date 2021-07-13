/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "fwd.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/runtime/runtime_types.hpp"

namespace fc::storage::blockchain {
  using primitives::tipset::Height;
  using vm::runtime::MessageReceipt;

  class ReceiptLoader {
   public:
    ReceiptLoader(TsLoadPtr ts_load, IpldPtr ipld);

    /**
     * Looks back for message receipt from topmost tipset up to lookback_limit
     * @param message_cid - message to look for
     * @param top_tipset_key - topmost tipset to start search from
     * @param lookback_limit - search depth, 0 means no depth, up to genesis
     * @return message receipt and tipset key where message is stored
     */
    outcome::result<boost::optional<std::pair<MessageReceipt, TipsetKey>>>
    searchBackForMessageReceipt(const CID &message_cid,
                                const TipsetKey &top_tipset_key,
                                size_t lookback_limit) const;

   private:
    TsLoadPtr ts_load_;
    IpldPtr ipld_;
  };

}  // namespace fc::storage::blockchain
