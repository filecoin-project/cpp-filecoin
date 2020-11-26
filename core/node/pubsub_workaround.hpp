/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PUBSUB_WORKAROUND_HPP
#define CPP_FILECOIN_PUBSUB_WORKAROUND_HPP

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>

#include "common/outcome.hpp"

namespace fc::node {
  //
  // TODO (artem): temporary workaround to strengthen pubsub graylist resistance
  //
  class PubsubWorkaround : private boost::noncopyable {
   public:
    PubsubWorkaround(std::shared_ptr<boost::asio::io_context> io_context,
                     const std::vector<libp2p::peer::PeerInfo> &bootstrap_list,
                     const libp2p::protocol::gossip::Config &gossip_config,
                     std::string network_name);

    outcome::result<libp2p::peer::PeerInfo> start(int port);

   private:
    class Impl;
    std::shared_ptr<Impl> impl_;
  };
}  // namespace fc::node

#endif  // CPP_FILECOIN_PUBSUB_WORKAROUND_HPP
