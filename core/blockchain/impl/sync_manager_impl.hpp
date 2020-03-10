/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP

#include "blockchain/sync_manager.hpp"

#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include "blockchain/syncer_state.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "blockchain/impl/sync_bucket_set.hpp"

namespace fc::blockchain::sync_manager {

  using SyncFunction = std::function<outcome::result<void>(
      std::reference_wrapper<const primitives::tipset::Tipset>)>;

  enum class SyncManagerError { SHUTTING_DOWN = 1 };

  struct SyncResult {
    primitives::tipset::Tipset tipset;
    bool success;
  };

  class SyncManagerImpl : public SyncManager,
                          public std::enable_shared_from_this<SyncManagerImpl> {
   public:
    using TipsetKey = primitives::tipset::TipsetKey;

    SyncManagerImpl(boost::asio::io_context &context,
                    SyncFunction sync_function);

    ~SyncManagerImpl() override = default;

    outcome::result<void> setPeerHead(PeerId peer_id,
                                      const Tipset &tipset) override;

    size_t syncedPeerCount() const;

    BootstrapState getBootstrapState() const;

    void setBootstrapState(BootstrapState state);

    bool isBootstrapped() const;

   private:
    outcome::result<void> processIncomingTipset(const Tipset &tipset);

    outcome::result<Tipset> selectSyncTarget();

    outcome::result<void> doWork();

    void processResult(const SyncResult &result);

    std::unordered_map<PeerId, Tipset> peer_heads_;
    BootstrapState state_;
    const uint64_t bootstrap_threshold_{1};
    std::deque<Tipset> sync_targets_;
    std::deque<SyncResult> sync_results_;
    std::vector<SyncerState> sync_states_;
    std::deque<Tipset> incoming_tipsets_;
    std::map<TipsetKey, Tipset> active_syncs_;
    SyncFunction sync_function_;
    common::Logger logger_;
  };

}  // namespace fc::blockchain

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::sync_manager, SyncManagerError);

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
