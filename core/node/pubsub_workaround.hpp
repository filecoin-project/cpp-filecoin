/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_PUBSUB_WORKAROUND_HPP
#define CPP_FILECOIN_PUBSUB_WORKAROUND_HPP

#include <boost/noncopyable.hpp>
#include <node/builder.hpp>

namespace fc::node {
  class PubsubWorkaround : private boost::noncopyable {
   public:
    PubsubWorkaround(std::shared_ptr<boost::asio::io_context> io_context,
                     const Config &config);

    outcome::result<libp2p::peer::PeerInfo> start(int port);
   private:
    class Impl;
    std::shared_ptr<Impl> impl_;
  };
}  // namespace fc::node

#endif  // CPP_FILECOIN_PUBSUB_WORKAROUND_HPP
