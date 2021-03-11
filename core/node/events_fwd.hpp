/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/signals2.hpp>

#include "fwd.hpp"

namespace fc::sync::events {
  using Connection = boost::signals2::scoped_connection;

  struct Events;

  // event types
  struct PeerConnected;
  struct PeerDisconnected;
  struct PeerLatency;
  struct TipsetFromHello;
  struct BlockFromPubSub;
  struct MessageFromPubSub;
  struct PossibleHead;
  struct CurrentHead;
  struct FatalError;
}  // namespace fc::sync::events
