/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_HPP
#define CPP_FILECOIN_GRAPHSYNC_HPP

#include <functional>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/subscription.hpp>

#include "common/buffer.hpp"
#include "common/outcome.hpp"

namespace fc::storage::ipfs::merkledag {
  class MerkleDagService;
}

namespace fc::storage::ipfs::graphsync {

  using libp2p::protocol::Subscription;

  /// Graphsync -> MerkleDagService bridge
  struct MerkleDagBridge {
    /// Creates a default bridge
    static std::shared_ptr<MerkleDagBridge> create(
        std::shared_ptr<merkledag::MerkleDagService> service);

    virtual ~MerkleDagBridge() = default;

    /// Forwards select call to the service or other backend
    virtual outcome::result<size_t> select(
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        std::function<bool(const common::Buffer &cid,
                           const common::Buffer &data)> handler) const = 0;
  };

  /// Response status codes
  enum ResponseStatusCode {
    // info - partial
    RS_REQUEST_ACKNOWLEDGED = 10,  //   Request Acknowledged. Working on it.
    RS_ADDITIONAL_PEERS = 11,      //   Additional Peers. PeerIDs in extra.
    RS_NOT_ENOUGH_GAS = 12,        //   Not enough vespene gas ($)
    RS_OTHER_PROTOCOL = 13,        //   Other Protocol - info in extra.
    RS_PARTIAL_RESPONSE =
        14,  //   Partial Response w/metadata, may incl. blocks

    // success - terminal
    RS_FULL_CONTENT = 20,     //   Request Completed, full content.
    RS_PARTIAL_CONTENT = 21,  //   Request Completed, partial content.

    // error - terminal
    RS_REJECTED = 30,        //   Request Rejected. NOT working on it.
    RS_TRY_AGAIN = 31,       //   Request failed, busy, try again later
    RS_REQUEST_FAILED = 32,  //   Request failed, for unknown reason.
    RS_LEGAL_ISSUES = 33,    //   Request failed, for legal reasons.
    RS_NOT_FOUND = 34,       //   Request failed, content not found.
  };

  /// Graphsync protocol interface
  class Graphsync {
   public:
    virtual ~Graphsync() = default;

    /// New nodes received go through this callback
    using BlockCallback =
        std::function<void(common::Buffer cid, common::Buffer data)>;

    /// Starts instance and subscribes to blocks.
    /// Instance will shut down as soon as this subscription is cancelled
    virtual Subscription start(std::shared_ptr<MerkleDagBridge> dag,
                               BlockCallback callback) = 0;

    /// Metadata pairs, is cid present or not
    using ResponseMetadata = std::unordered_map<common::Buffer, bool>;

    /// Request progress subscription data
    using RequestProgressCallback =
        std::function<void(ResponseStatusCode code, ResponseMetadata meta)>;

    /// Makes request. Request will be cancelled if subscription is cancelled
    virtual Subscription makeRequest(
        libp2p::peer::PeerId peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        std::unordered_set<common::Buffer> dont_send_cids) = 0;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_HPP
