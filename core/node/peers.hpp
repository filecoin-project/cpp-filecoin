/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_PEERS_HPP
#define CPP_FILECOIN_SYNC_PEERS_HPP

#include <map>
#include <unordered_map>

#include "events_fwd.hpp"

#include <libp2p/peer/peer_id.hpp>

namespace fc::sync {
  class Peers {
   public:
    using PeerId = libp2p::peer::PeerId;
    using Rating = int64_t;
    using PeersAndRatings = std::unordered_map<PeerId, Rating>;
    using RatingChangeFunction =
        std::function<Rating(Rating current_rating, Rating delta)>;
    using RatingChangeLatencyFunction =
        std::function<Rating(Rating current_rating, int64_t latency)>;

    void start(std::vector<std::string> protocols_required,
               events::Events &events,
               RatingChangeFunction rating_fn = RatingChangeFunction{},
               RatingChangeLatencyFunction rating_latency_fn =
                   RatingChangeLatencyFunction{});

    bool isConnected(const PeerId &peer) const;

    const PeersAndRatings &getAllPeers() const;

    boost::optional<PeerId> selectBestPeer() const;

    void changeRating(const PeerId &peer, Rating delta);

   private:
    void removeFromRatings(const PeersAndRatings::iterator &it);

    void changeRating(PeersAndRatings::iterator &it, Rating new_rating);

    PeersAndRatings peers_;
    std::multimap<Rating, PeerId, std::greater<>> ratings_;
    std::vector<std::string> protocols_;
    RatingChangeFunction rating_fn_;
    RatingChangeLatencyFunction rating_latency_fn_;
    events::Connection peer_connected_event_;
    events::Connection peer_disconnected_event_;
    events::Connection peer_latency_event_;
  };
}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_PEERS_HPP
