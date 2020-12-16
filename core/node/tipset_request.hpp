/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_TIPSET_REQUEST_HPP
#define CPP_FILECOIN_SYNC_TIPSET_REQUEST_HPP

#include "common.hpp"

namespace fc::sync {

  class ChainDb;

  class TipsetRequest {
   public:
    /// Subchain was indexed from head to bottom if success
    struct Result {
      /// Peer they came from
      boost::optional<PeerId> from;

      int64_t delta_rating = 0;

      /// Not 0 if error occured
      std::error_code error;

      /// Highest tipset loaded
      TipsetCPtr head;

      Height next_target_height = 0;

      /// Lower subschain to be loaded
      boost::optional<TipsetKey> next_target;

      /// If false, only parents are indexed
      bool head_indexed = false;
    };

    TipsetRequest(ChainDb &db,
                  libp2p::Host &host,
                  libp2p::protocol::Scheduler &scheduler,
                  Ipld &ipld,
                  PeerId peer,
                  std::vector<CID> blocks,
                  uint64_t depth,
                  uint64_t timeoutMsec,
                  bool index_head_tipset,
                  std::function<void(Result)> callback);

    ~TipsetRequest();

    void cancel();

   private:
    class Impl;
    std::shared_ptr<Impl> impl_;
  };

}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_TIPSET_REQUEST_HPP
