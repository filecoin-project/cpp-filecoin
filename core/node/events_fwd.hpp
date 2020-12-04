/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_NODE_FWD_HPP
#define CPP_FILECOIN_NODE_FWD_HPP

#include <boost/signals2.hpp>

#include "fwd.hpp"

namespace fc::sync {
  namespace events {
    using Connection = boost::signals2::scoped_connection;

    struct Events;

    // event types
    struct PeerConnected;
    struct PeerDisconnected;
    struct PeerLatency;
    struct TipsetFromHello;
    struct BlockFromPubSub;
    struct MessageFromPubSub;
    struct BlockStored;
    struct TipsetStored;
    struct PossibleHead;
    struct HeadInterpreted;
    struct CurrentHead;
  }  // namespace events
}  // namespace fc::sync

#endif  // CPP_FILECOIN_NODE_FWD_HPP
