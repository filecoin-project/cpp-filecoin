/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP
#define CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP

#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "markets/retrieval/common_types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/face/persistent_map.hpp"

namespace fc::markets::discovery {

  using common::Buffer;
  using primitives::address::Address;
  using retrieval::RetrievalPeer;
  using Datastore = storage::face::PersistentMap<Buffer, Buffer>;

  /**
   * Deal discovery
   */
  class Discovery {
   public:
    explicit Discovery(std::shared_ptr<Datastore> datastore);

    virtual ~Discovery() = default;

    /**
     * Add peer
     * @param cid - deal proposal cid
     * @param peer - peer to add
     * @return error if happens
     */
    outcome::result<void> addPeer(const CID &cid, const RetrievalPeer &peer);

    /**
     * Get peers by proposal cid
     * @param cid - deal proposal cid
     * @return vector of peers
     */
    outcome::result<std::vector<RetrievalPeer>> getPeers(const CID &cid) const;

   private:
    std::shared_ptr<Datastore> datastore_;
  };

}  // namespace fc::markets::discovery

#endif  // CPP_FILECOIN_CORE_MARKETS_DISCOVERY_DISCOVERY_HPP
