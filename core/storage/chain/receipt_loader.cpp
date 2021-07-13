/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/receipt_loader.hpp"
#include "primitives/tipset/load.hpp"

namespace fc::storage::blockchain {

  ReceiptLoader::ReceiptLoader(TsLoadPtr ts_load, IpldPtr ipld)
      : ts_load_{std::move(ts_load)}, ipld_{std::move(ipld)} {}

  outcome::result<boost::optional<std::pair<MessageReceipt, TipsetKey>>>
  ReceiptLoader::searchBackForMessageReceipt(const CID &message_cid,
                                             const TipsetKey &top_tipset_key,
                                             size_t lookback_limit) const {
    boost::optional<MessageReceipt> found;
    auto tipset_key = top_tipset_key;
    OUTCOME_TRY(tipset, ts_load_->load(tipset_key));
    Height height_limit = tipset->height() - lookback_limit;

    while ((lookback_limit == 0) || (height_limit < tipset->height())) {
      // reached genesis
      if (tipset->height() == 0) {
        break;
      }
      adt::Array<MessageReceipt> receipts{tipset->getParentMessageReceipts(),
                                          ipld_};
      OUTCOME_TRY(parent, ts_load_->load(tipset->getParents()));
      OUTCOME_TRY(parent->visitMessages(
          {ipld_, true, false},
          [&](auto i, auto, auto &msg_cid, auto, auto)
              -> outcome::result<void> {
            if (!found.has_value() && (msg_cid == message_cid)) {
              OUTCOME_TRYA(found, receipts.get(i));
            }
            return outcome::success();
          }));
      if (found.has_value()) {
        return std::make_pair(found.value(), tipset_key);
      }
      tipset_key = tipset->getParents();
      OUTCOME_TRYA(tipset, ts_load_->load(tipset_key));
    }

    return boost::none;
  }
}  // namespace fc::storage::blockchain
