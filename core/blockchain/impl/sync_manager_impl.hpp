/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP

#include "blockchain/sync_manager.hpp"

#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include "blockchain/impl/sync_bucket_set.hpp"
#include "blockchain/syncer_state.hpp"
#include "common/logger.hpp"
#include "common/outcome.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::blockchain::sync_manager {

  using SyncFunction = std::function<outcome::result<void>(
      std::reference_wrapper<const primitives::tipset::Tipset>)>;

  enum class SyncManagerError { SHUTTING_DOWN = 1, NO_SYNC_TARGET };

  struct SyncResult {
    primitives::tipset::Tipset tipset;
    outcome::result<void> success;
  };

  class SyncManagerImpl : public SyncManager,
                          public std::enable_shared_from_this<SyncManagerImpl> {
   public:
    const static size_t kBootstrapThresholdDefault = 1;

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

    outcome::result<void> processSyncTargets(Tipset ts);

    outcome::result<void> doSync();

    outcome::result<void> processResult(const SyncResult &result);
    std::unordered_map<PeerId, Tipset> peer_heads_;
    BootstrapState state_;
    const uint64_t bootstrap_threshold_{kBootstrapThresholdDefault};
    std::deque<Tipset> sync_targets_;
    std::deque<SyncResult> sync_results_;
    std::deque<Tipset> incoming_tipsets_;
    std::unordered_map<TipsetKey, Tipset> active_syncs_;
    boost::optional<SyncTargetBucket> next_sync_target_;
    SyncBucketSet sync_queue_{gsl::span<Tipset>{}};
    SyncBucketSet active_sync_tips_{gsl::span<Tipset>{}};
    SyncFunction sync_function_;
    common::Logger logger_;
  };

}  // namespace fc::blockchain::sync_manager

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::sync_manager, SyncManagerError);

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
