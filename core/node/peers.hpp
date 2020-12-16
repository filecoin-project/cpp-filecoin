/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_PEERS_HPP
#define CPP_FILECOIN_SYNC_PEERS_HPP

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "events_fwd.hpp"

#include <libp2p/peer/peer_id.hpp>

namespace fc::sync {
  class Peers {
   public:
    using PeerId = libp2p::peer::PeerId;
    using Rating = int64_t;
    using PeersAndRatings = std::unordered_map<PeerId, Rating>;
    using RatingsAndPeers = std::multimap<Rating, PeerId, std::greater<>>;
    using RatingChangeFunction =
        std::function<Rating(Rating current_rating, Rating delta)>;
    using RatingChangeLatencyFunction =
        std::function<Rating(Rating current_rating, int64_t latency)>;
    using PeerSelectFunction =
        std::function<bool(const std::set<std::string> &protocols)>;

    void start(std::shared_ptr<libp2p::Host> host,
               events::Events &events,
               PeerSelectFunction select_fn,
               RatingChangeFunction rating_fn = RatingChangeFunction{},
               RatingChangeLatencyFunction rating_latency_fn =
                   RatingChangeLatencyFunction{});

    bool isConnected(const PeerId &peer) const;

    const PeersAndRatings &getPeers() const;

    const RatingsAndPeers &getPeersWithRatings() const;

    boost::optional<PeerId> selectBestPeer(
        const std::unordered_set<PeerId> &prefer) const;

    void changeRating(const PeerId &peer, Rating delta);

   private:
    void removeFromRatings(const PeersAndRatings::iterator &it);

    void changeRating(PeersAndRatings::iterator &it, Rating new_rating);

    std::shared_ptr<libp2p::Host> host_;
    PeersAndRatings peers_;
    RatingsAndPeers ratings_;
    PeerSelectFunction select_fn_;
    RatingChangeFunction rating_fn_;
    RatingChangeLatencyFunction rating_latency_fn_;
    events::Connection peer_connected_event_;
    events::Connection peer_disconnected_event_;
    events::Connection peer_latency_event_;
  };
}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_PEERS_HPP
