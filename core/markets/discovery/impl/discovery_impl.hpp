/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "markets/discovery/discovery.hpp"

namespace fc::markets::discovery {

  class DiscoveryImpl : public Discovery {
   public:
    explicit DiscoveryImpl(std::shared_ptr<Datastore> datastore);

    outcome::result<void> addPeer(const CID &cid,
                                  const RetrievalPeer &peer) override;

    outcome::result<std::vector<RetrievalPeer>> getPeers(
        const CID &cid) const override;

   private:
    std::shared_ptr<Datastore> datastore_;
  };

}  // namespace fc::markets::discovery
