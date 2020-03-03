/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
#define CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP

#include "blockchain/sync_manager.hpp"

#include <unordered_map>

#include <boost/asio/io_context.hpp>
#include <boost/fiber/buffered_channel.hpp>
#include <boost/fiber/unbuffered_channel.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "blockchain/syncer_state.hpp"
#include "primitives/tipset/tipset.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::blockchain {
  // type SyncManager struct {
  //  lk        sync.Mutex
  //      peerHeads map[peer.ID]*types.TipSet
  //
  //      bssLk          sync.Mutex
  //      bootstrapState int
  //
  //      bspThresh int
  //
  //      incomingTipSets chan *types.TipSet
  //      syncTargets     chan *types.TipSet
  //      syncResults     chan *syncResult
  //
  //  syncStates []*SyncerState
  //
  //      doSync func(context.Context, *types.TipSet) error
  //
  //      stop chan struct{}
  //
  //  // Sync Scheduler fields
  //  activeSyncs    map[types.TipSetKey]*types.TipSet
  //      syncQueue      syncBucketSet
  //      activeSyncTips syncBucketSet
  //      nextSyncTarget *syncTargetBucket
  //      workerChan     chan *types.TipSet
  //

  using SyncFunction = std::function<outcome::result<void>(
      std::reference_wrapper<const primitives::tipset::Tipset>)>;

  template <class T, size_t size = 0>
  using BufferedChannel = boost::fibers::buffered_channel<T>;

  template <class T>
  using UnbufferedChannel = boost::fibers::unbuffered_channel<T>;

  class SyncManagerImpl : public SyncManager,
                          public std::enable_shared_from_this<SyncManagerImpl> {
   public:
    using PeerId = libp2p::peer::PeerId;
    using Tipset = primitives::tipset::Tipset;
    using TipsetKey = primitives::tipset::TipsetKey;

    SyncManagerImpl(boost::asio::io_context &context,
                    SyncFunction sync_function);

    ~SyncManagerImpl() override = default;

    outcome::result<void> start() override;

    outcome::result<void> stop() override;

    outcome::result<void> setPeerHead() override;

    void workerMethod(int id) override;

    size_t syncedPeerCount() const override;

    BootstrapState getBootstrapState() const override;

    void setBootstrapState(BootstrapState state) override;

    bool isBootstrapped() const override;

   private:
    void scheduleWorker(int id);

    static const size_t kSyncWorkerCount = 3u;

    boost::asio::io_context &context_;

    std::unordered_map<PeerId, Tipset> peer_heads_;
    BootstrapState state_;
    uint64_t bootstrap_threshold_;
    UnbufferedChannel<Tipset> sync_targets_;
    UnbufferedChannel<SyncResult> sync_results_;
    std::vector<SyncerState> sync_states_;
    UnbufferedChannel<Tipset> incoming_tipsets_;
    std::map<TipsetKey, Tipset> active_syncs_;
    SyncFunction sync_function_;
  };

}  // namespace fc::blockchain

#endif  // CPP_FILECOIN_CORE_BLOCKCHAIN_IMPL_SYNC_MANAGER_IMPL_HPP
