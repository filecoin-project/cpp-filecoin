/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/signals2/connection.hpp>

#include "node/fwd.hpp"

namespace fc::peermgr {
  using hello::Hello;
  using libp2p::Host;
  using libp2p::protocol::Identify;
  using libp2p::protocol::IdentifyDelta;
  using libp2p::protocol::IdentifyPush;

  struct PeerMgr {
    PeerMgr(std::shared_ptr<Host> host,
            std::shared_ptr<Identify> identify,
            std::shared_ptr<IdentifyPush> identify_push,
            std::shared_ptr<IdentifyDelta> identify_delta,
            std::shared_ptr<Hello> hello);

    boost::signals2::connection identify_sub;
  };
}  // namespace fc::peermgr
