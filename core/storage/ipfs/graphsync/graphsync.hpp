/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_HPP
#define CPP_FILECOIN_GRAPHSYNC_HPP

#include <functional>

#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/common/subscription.hpp>

#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::storage::ipfs::merkledag {
  class MerkleDagService;
}

namespace fc::storage::ipfs::graphsync {

  /// Subscription to any data stream, borrowed from libp2p
  using libp2p::protocol::Subscription;

  /// Graphsync -> MerkleDagService bridge
  struct MerkleDagBridge {
    /// Creates a default bridge
    /// \param service Existing MerkleDAG service
    /// \return Bridge object
    static std::shared_ptr<MerkleDagBridge> create(
        std::shared_ptr<merkledag::MerkleDagService> service);

    virtual ~MerkleDagBridge() = default;

    /// Forwards select call to the service or other backend
    /// \param cid Root CID
    /// \param selector IPLD selector
    /// \param handler Data handler, returns false if further search is no
    /// longer required
    /// \return Count of data blocks passed through handler or error
    virtual outcome::result<size_t> select(
        const CID &cid,
        gsl::span<const uint8_t> selector,
        std::function<bool(const CID &cid, const common::Buffer &data)> handler)
        const = 0;
  };

  /// Response status codes. Positive values are received from wire,
  /// negative are internal. Terminal codes end request-response session
  enum ResponseStatusCode {
    // internal codes - terminal
    RS_NO_PEERS = -1,          // no peers: cannot find peer to connect to
    RS_CANNOT_CONNECT = -2,    // error during outbound connection establishment
    RS_TIMEOUT = -3,           // timeout occured in p2p communication
    RS_CONNECTION_ERROR = -4,  // network error (due to connection)
    RS_INTERNAL_ERROR = -5,    // internal error (due to local components)
    RS_REJECTED_LOCALLY = -6,  // request was rejected by local side
    RS_SLOW_STREAM = -7,       // slow stream: outbound buffers overflow

    // Other response codes are received from the network

    // info - partial
    RS_REQUEST_ACKNOWLEDGED = 10,  //   Request Acknowledged. Working on it.
    RS_ADDITIONAL_PEERS = 11,      //   Additional Peers. PeerIDs in extra.
    RS_NOT_ENOUGH_GAS = 12,        //   Not enough vespene gas ($)
    RS_OTHER_PROTOCOL = 13,        //   Other Protocol - info in extra.
    RS_PARTIAL_RESPONSE = 14,      //   Partial Response w/metadata

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

  /// Returns true if the status code passed is terminal, i.e. no more data will
  /// be sent to the subscription. See ResponseStatusCode comments
  bool isTerminal(ResponseStatusCode code);

  /// Converts status code to string repr
  std::string statusCodeToString(ResponseStatusCode code);

  /// Graphsync protocol interface
  class Graphsync {
   public:
    virtual ~Graphsync() = default;

    /// New nodes received go through this callback
    using BlockCallback = std::function<void(CID cid, common::Buffer data)>;

    /// Starts instance and subscribes to blocks
    /// \param dag Object which allows to select in local storage as per
    /// incoming requests
    /// \param callback Callback which receives blocks of data from the network
    virtual void start(std::shared_ptr<MerkleDagBridge> dag,
                       BlockCallback callback) = 0;

    /// Stops the instance. Active requests will be cancelled and return
    /// RS_REJECTED_LOCALLY to their callbacks
    virtual void stop() = 0;

    /// Request progress subscription data
    using RequestProgressCallback = std::function<void(
        ResponseStatusCode code, std::vector<Extension> extensions)>;

    /// Initiates a new request to graphsync network
    /// \param peer Peer ID
    /// \param address Optional peer network address
    /// \param root_cid Root CID of the request
    /// \param selector IPLD selector
    /// \param extensions - extension data
    /// \param callback A callback which keeps track of request progress
    /// \return Subscription object. Request is cancelled as soon as
    /// this subscription is cancelled or goes out of scope
    virtual Subscription makeRequest(
        const libp2p::peer::PeerId &peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        const CID &root_cid,
        gsl::span<const uint8_t> selector,
        const std::vector<Extension> &extensions,
        RequestProgressCallback callback) = 0;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_HPP
