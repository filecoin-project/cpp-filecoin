/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
#define CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP

#include <map>

#include "message.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

namespace fc::storage::ipfs::graphsync {

  class Connector;

  class LocalRequests : public Subscription::Source {
   public:
    using AsyncPostFunction = std::function<void(std::function<void()>)>;

    LocalRequests(AsyncPostFunction async_post);

    /// Cancels all active requests
    void stop();

    Subscription makeRequest(
        const libp2p::peer::PeerId& peer,
        boost::optional<libp2p::multi::Multiaddress> address,
        gsl::span<const uint8_t> root_cid,
        gsl::span<const uint8_t> selector,
        bool need_metadata,
        const std::vector<CID>& dont_send_cids,
        Graphsync::RequestProgressCallback callback);

    void onResponses(const SessionPtr &from,
                     const std::vector<Message::Response> &responses);

   private:
    void unsubscribe(uint64_t ticket) override;

    SessionPtr findOrCreateSession(const PeerId& peer);

    void flushOutMessages(const SessionPtr& session);

    void closeSession(SessionPtr session);

    void asyncPostError(SessionPtr from, outcome::result<void> error);

    void onConnectionError(const SessionPtr& from, outcome::result<void> error);

    std::shared_ptr<Connector> connector_;

    std::unordered_map<PeerId, SessionPtr> sessions_by_peer_;
    std::unordered_map<uint64_t, SessionPtr> sessions_by_ticket_;
    std::unordered_map<uint64_t, int> request_by_ticket_;

    uint64_t current_ticket_ = 0;
    uint64_t current_session_id_ = 1;
    int current_request_id_ = 0;
  };

}  // namespace fc::storage::ipfs::graphsync

#endif  // CPP_FILECOIN_GRAPHSYNC_LOCAL_REQUESTS_HPP
