/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
#define CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP

#include "storage/chain/chain_store.hpp"

#include <libp2p/protocol/common/scheduler.hpp>

#include "blockchain/block_validator/block_validator.hpp"
#include "blockchain/weight_calculator.hpp"
#include "sync/common.hpp"

namespace fc::sync {
  class ChainDb;
  class Syncer;
  class PeerManager;
}  // namespace fc::sync

namespace fc::storage::blockchain {

  //  using ::fc::blockchain::block_validator::BlockValidator;
  using ::fc::blockchain::weight::WeightCalculator;
  using sync::BigInt;
  //  using ipfs::IpfsDatastore;
  //  using libp2p::peer::PeerId;
  //  using primitives::tipset::Tipset;
  //  using primitives::tipset::TipsetHash;
  //  using primitives::tipset::TipsetKey;

  using primitives::tipset::TipsetCreator;

  using sync::Height;

  class ChainStoreImpl : public ChainStore,
                         public std::enable_shared_from_this<ChainStoreImpl> {
   public:
    ChainStoreImpl(ChainStoreImpl &&other) = default;

    ChainStoreImpl(/*std::shared_ptr<IpfsDatastore> data_store,
                   std::shared_ptr<BlockValidator> block_validator,
                   std::shared_ptr<WeightCalculator> weight_calculator,
                   std::shared_ptr<sync::TipsetLoader> tipset_loader,
                   std::shared_ptr<sync::PeerManager> peer_manager*/);


    // TODO start peer manager here, wire callbacks from gossip,
    //  peer manager & syncer & epoch timer
    outcome::result<void> start() override;

    outcome::result<void> addBlock(const BlockHeader &block) override;

    outcome::result<TipsetCPtr> loadTipset(const TipsetHash &hash) override;

    outcome::result<TipsetCPtr> loadTipset(const TipsetKey &key) override;

    outcome::result<TipsetCPtr> loadTipsetByHeight(uint64_t height) override;

    outcome::result<TipsetCPtr> heaviestTipset() const override;

    using connection_t = boost::signals2::connection;
    using HeadChangeSignature = void(const HeadChange &);

    connection_t subscribeHeadChanges(
        const std::function<HeadChangeSignature> &subscriber) override;

    const std::string &getNetworkName() const override;

    const CID &genesisCID() const override;

   private:
    // heads changed callback from ChainDb
    void onHeadsChanged(std::vector<TipsetHash> removed,
                        std::vector<TipsetHash> added);

    std::pair<TipsetCPtr, BigInt> chooseNewHead(
        const std::vector<TipsetHash> &heads_added);

    void switchToHead(TipsetCPtr new_head, BigInt new_weight);

    void onChainEpochTimer(Height new_epoch);

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler_;

    std::string network_name_;
    std::shared_ptr<sync::ChainDb> chain_db_;
    std::shared_ptr<sync::PeerManager> peer_manager_;
    std::shared_ptr<WeightCalculator> weight_calculator_;
    std::shared_ptr<sync::Syncer> syncer_;

    TipsetCPtr head_;
    TipsetHash heads_parent_;
    BigInt current_weight_;

    // TODO таймер через шедулер на переключение эпох

    Height current_epoch_ = 0;


    // creating head candidates for current epoch, parent->possible_head
    std::map<TipsetHash, TipsetCreator> head_candidates_;


    //
    //    storage::indexdb::Branches heads_;
    //    storage::indexdb::BranchInfo head_branch_;
    //    boost::optional<Tipset> heaviest_tipset_;  ///< current heaviest
    //    tipset primitives::BigInt heaviest_weight_{0};    ///< current
    //    heaviest weight boost::optional<Tipset> genesis_;          ///<
    //    genesis block boost::optional<CID> genesis_cid_;
    //    boost::optional<PeerId> current_peer_;
    //    boost::optional<Tipset> tipset_in_production_;
    //    boost::optional<TipsetHash> tipset_in_sync_;
    //    storage::indexdb::BranchId syncing_branch_ =
    //    storage::indexdb::kNoBranch;
    //
    //    // std::unordered_map<uint64_t, std::vector<CID>> tipsets_;
    //    // mutable std::unordered_map<TipsetKey, Tipset> tipsets_cache_;

    /// when head tipset changes, need to notify all subscribers
    boost::signals2::signal<HeadChangeSignature> head_change_signal_;

    // common::Logger logger_;
  };
}  // namespace fc::storage::blockchain

#endif  // CPP_FILECOIN_CORE_STORAGE_CHAIN_CHAIN_STORE_IMPL_HPP
