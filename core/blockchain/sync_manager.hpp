/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP

#include <libp2p/peer/peer_id.hpp>
#include "common/outcome.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain {

  enum class BootstrapState : int {
    STATE_INIT = 0,
    STATE_SELECTED,
    STATE_SCHEDULED,
    STATE_COMPLETE,
  };

  /** @brief sync manager */
  class SyncManager {
   public:
    using PeerId = libp2p::peer::PeerId;
    using Tipset = primitives::tipset::Tipset;

    virtual ~SyncManager() = default;

    virtual outcome::result<void> setPeerHead(PeerId peer_id,
                                              const Tipset &tipset) = 0;
  };
}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
