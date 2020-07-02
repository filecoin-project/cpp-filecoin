/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP
#define CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP

#include <libp2p/peer/peer_info.hpp>
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::markets::discovery {

  using common::Buffer;
  using libp2p::peer::PeerInfo;
  using Datastore = fc::storage::face::PersistentMap<Buffer, Buffer>;

  /**
   * Storage/retrieval markets peer resolver.
   * Storage market adds peers on deal by payload root cid. Later, retrieval
   * market can find provider peer by payload cid interested in.
   */
  class Discovery {
   public:
    explicit Discovery(std::shared_ptr<Datastore> datastore);

    virtual ~Discovery() = default;

    /**
     * Add peer
     * @param cid - payload root cid
     * @param peer - peer to add
     * @return error if happens
     */
    outcome::result<void> addPeer(const CID &cid, const PeerInfo &peer);

    /**
     * Get peers by proposal cid
     * @param cid - payload root cid
     * @return vector of peers housing payload with cid
     */
    outcome::result<std::vector<PeerInfo>> getPeers(const CID &cid) const;

   private:
    std::shared_ptr<Datastore> datastore_;
  };

}  // namespace fc::markets::discovery

#endif  // CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP
