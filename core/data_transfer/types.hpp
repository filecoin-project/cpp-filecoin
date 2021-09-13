/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>

#include "primitives/cid/cid.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"
#include "storage/ipld/selector.hpp"

namespace fc::data_transfer {
  using libp2p::peer::PeerInfo;
  using storage::ipld::Selector;

  /**
   * TransferID is an identifier for a data transfer, shared between
   * request/responder and unique to the requester
   */
  using TransferId = uint64_t;
}  // namespace fc::data_transfer
