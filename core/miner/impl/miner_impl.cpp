/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner_impl.hpp"
#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"
#include "miner/storage_fsm/impl/events_impl.hpp"
#include "miner/storage_fsm/impl/sealing_impl.hpp"
#include "miner/storage_fsm/impl/tipset_cache_impl.hpp"
#include "miner/storage_fsm/precommit_policy.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "vm/actor/builtin/miner/policy.hpp"

namespace fc::miner {
  using mining::BasicPreCommitPolicy;
  using mining::Events;
  using mining::EventsImpl;
  using mining::kGlobalChainConfidence;
  using mining::PreCommitPolicy;
  using mining::SealingImpl;
  using mining::TipsetCache;
  using mining::TipsetCacheImpl;
  using primitives::tipset::TipsetKey;
  using vm::actor::builtin::miner::kMaxSectorExpirationExtension;
  using vm::actor::builtin::miner::kWPoStProvingPeriod;

  MinerImpl::MinerImpl(std::shared_ptr<Api> api,
                       Address miner_address,
                       Address worker_address,
                       std::shared_ptr<Counter> counter,
                       std::shared_ptr<Manager> sector_manager,
                       std::shared_ptr<boost::asio::io_context> context)
      : api_{std::move(api)},
        miner_address_{std::move(miner_address)},
        worker_address_{std::move(worker_address)},
        counter_{std::move(counter)},
        sector_manager_{std::move(sector_manager)},
        context_{std::move(context)} {}

  outcome::result<void> MinerImpl::run() {
    OUTCOME_TRY(runPreflightChecks());

    // use empty TipsetKey
    OUTCOME_TRY(deadline_info,
                api_->StateMinerProvingDeadline(miner_address_, TipsetKey{}));

    std::shared_ptr<TipsetCache> tipset_cache =
        std::make_shared<TipsetCacheImpl>(
            2 * kGlobalChainConfidence,
            [=](auto h) { return api_->ChainGetTipSetByHeight(h, {}); });
    std::shared_ptr<Events> events =
        std::make_shared<EventsImpl>(api_, tipset_cache);
    OUTCOME_TRY(events->subscribeHeadChanges());
    std::shared_ptr<PreCommitPolicy> precommit_policy =
        std::make_shared<BasicPreCommitPolicy>(
            api_,
            kMaxSectorExpirationExtension - 2 * kWPoStProvingPeriod,
            deadline_info.period_start % kWPoStProvingPeriod);
    sealing_ = std::make_shared<SealingImpl>(api_,
                                             events,
                                             miner_address_,
                                             counter_,
                                             sector_manager_,
                                             precommit_policy,
                                             context_);
    OUTCOME_TRY(sealing_->run());

    return outcome::success();
  }

  void MinerImpl::stop() {
    sealing_->stop();
  }

  outcome::result<std::shared_ptr<SectorInfo>> MinerImpl::getSectorInfo(
      SectorNumber sector_id) const {
    return sealing_->getSectorInfo(sector_id);
  }

  outcome::result<PieceAttributes> MinerImpl::addPieceToAnySector(
      UnpaddedPieceSize size, const PieceData &piece_data, DealInfo deal) {
    return sealing_->addPieceToAnySector(size, piece_data, deal);
  }

  Address MinerImpl::getAddress() const {
    return sealing_->getAddress();
  }

  outcome::result<void> MinerImpl::runPreflightChecks() {
    OUTCOME_TRY(has, api_->WalletHas(worker_address_));
    if (!has) {
      return MinerError::kWorkerNotFound;
    }
    return outcome::success();
  }

}  // namespace fc::miner

OUTCOME_CPP_DEFINE_CATEGORY(fc::miner, MinerError, e) {
  using E = fc::miner::MinerError;

  switch (e) {
    case E::kWorkerNotFound:
      return "MinerError: key for worker not found in local wallet";
    default:
      return "MinerError: unknown error";
  }
}
