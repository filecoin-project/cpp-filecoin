/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_FWD_HPP
#define CPP_FILECOIN_SYNC_FWD_HPP

#include <boost/signals2.hpp>

#include <libp2p/peer/peer_id.hpp>

#include "primitives/big_int.hpp"
#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset.hpp"
#include "vm/message/message.hpp"

namespace libp2p {
  struct Host;
}

namespace fc::sync {
  using libp2p::peer::PeerId;
  using primitives::BigInt;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetKey;
  using primitives::block::BlockHeader;
  using primitives::block::BlockWithCids;
  using primitives::block::BlockWithMessages;
  using primitives::block::SignedMessage;
  using vm::message::UnsignedMessage;
  using crypto::signature::Signature;
  using TipsetCPtr = std::shared_ptr<const Tipset>;
  using primitives::tipset::TipsetHash;
  using BranchId = uint64_t;
  using Height = uint64_t;

  constexpr BranchId kNoBranch = 0;
  constexpr BranchId kGenesisBranch = 1;

  namespace events {
    using Connection = boost::signals2::connection;

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
  }
}  // namespace fc::sync

#endif  // CPP_FILECOIN_SYNC_FWD_HPP
