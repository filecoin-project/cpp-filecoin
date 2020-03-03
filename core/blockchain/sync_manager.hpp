/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP

#include "common/outcome.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::blockchain {

  using SyncResult = outcome::result<primitives::tipset::Tipset>;

  enum class BootstrapState : int {
    STATE_INIT = 0,
    STATE_SELECTED = 1,
    STATE_SCHEDULED = 2,
    STATE_COMPLETE = 3
  };

  /** @brief sync manager */
  class SyncManager {
   public:
    virtual ~SyncManager() = default;

    virtual outcome::result<void> start() = 0;
    virtual outcome::result<void> stop() = 0;
    virtual outcome::result<void> setPeerHead() = 0;

    virtual void workerMethod(int id) = 0;

    virtual size_t syncedPeerCount() const = 0;

    virtual BootstrapState getBootstrapState() const = 0;

    virtual void setBootstrapState(BootstrapState state) = 0;

    virtual bool isBootstrapped() const = 0;
  };
}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_SYNC_MANAGER_HPP
