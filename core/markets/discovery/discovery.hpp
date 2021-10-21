/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "markets/retrieval/types.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::markets::discovery {
  using markets::retrieval::RetrievalPeer;
  using Datastore = fc::storage::face::PersistentMap<Bytes, Bytes>;

  /**
   * Storage/retrieval markets peer resolver.
   * Storage market adds peers on deal by payload root cid. Later, retrieval
   * market can find provider peer by payload cid was interested in.
   */
  class Discovery {
   public:
    virtual ~Discovery() = default;

    /**
     * Add peer
     * @param cid - payload root cid
     * @param peer - peer to add
     * @return error if happens
     */
    virtual outcome::result<void> addPeer(const CID &cid,
                                          const RetrievalPeer &peer) = 0;

    /**
     * Get peers by proposal cid
     * @param cid - payload root cid
     * @return vector of peers housing payload with cid
     */
    virtual outcome::result<std::vector<RetrievalPeer>> getPeers(
        const CID &cid) const = 0;
  };

}  // namespace fc::markets::discovery
