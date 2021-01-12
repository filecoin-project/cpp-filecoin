/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_HEAD_CONSTRUCTOR_HPP
#define CPP_FILECOIN_SYNC_HEAD_CONSTRUCTOR_HPP

#include "common.hpp"

namespace fc::sync {

  /// Active object which constructs head tipsets from blocks coming from
  /// pubsub. Also it keeps track of Hello protocol messages.
  /// Emits PossibleHead events
  class HeadConstructor {
   public:
    void start(std::shared_ptr<events::Events> events);

    void blockFromApi(const CID &block_cid, const BlockHeader &block);

   private:
    void tryAppendBlock(boost::optional<PeerId> source,
                        const CID &block_cid,
                        const BlockHeader &header);

    BigInt min_weight_;
    Height current_height_ = 0;
    std::map<TipsetHash, primitives::tipset::TipsetCreator> candidates_;

    std::shared_ptr<events::Events> events_;
    events::Connection block_from_pubsub_event_;
    events::Connection tipset_from_hello_event_;
    events::Connection current_head_event_;
  };
}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_HEAD_CONSTRUCTOR_HPP
