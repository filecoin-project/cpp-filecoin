/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner_impl.hpp"
#include "miner/address_selector.hpp"
#include "miner/storage_fsm/impl/basic_precommit_policy.hpp"
#include "miner/storage_fsm/impl/events_impl.hpp"
#include "miner/storage_fsm/impl/sealing_impl.hpp"
#include "miner/storage_fsm/impl/tipset_cache_impl.hpp"
#include "miner/storage_fsm/precommit_batcher.hpp"
#include "miner/storage_fsm/precommit_policy.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::miner {
  using api::MinerInfo;
  using mining::BasicPreCommitPolicy;
  using mining::Events;
  using mining::EventsImpl;
  using mining::kGlobalChainConfidence;
  using mining::PreCommitBatcher;
  using mining::PreCommitBatcherImpl;
  using mining::PreCommitPolicy;
  using mining::SealingImpl;
  using mining::SelectAddress;
  using mining::TipsetCache;
  using mining::TipsetCacheImpl;
  using mining::types::FeeConfig;
  using primitives::TokenAmount;
  using primitives::tipset::TipsetKey;
  using vm::actor::builtin::types::miner::kMaxSectorExpirationExtension;
  using vm::actor::builtin::types::miner::kWPoStProvingPeriod;

  MinerImpl::MinerImpl(std::shared_ptr<FullNodeApi> api,
                       std::shared_ptr<Sealing> sealing)
      : api_{std::move(api)}, sealing_{std::move(sealing)} {}

  outcome::result<std::shared_ptr<SectorInfo>> MinerImpl::getSectorInfo(
      SectorNumber sector_id) const {
    return sealing_->getSectorInfo(sector_id);
  }

  outcome::result<PieceAttributes> MinerImpl::addPieceToAnySector(
      UnpaddedPieceSize size, PieceData piece_data, DealInfo deal) {
    return sealing_->addPieceToAnySector(size, std::move(piece_data), deal);
  }

  Address MinerImpl::getAddress() const {
    return sealing_->getAddress();
  }

  std::shared_ptr<Sealing> MinerImpl::getSealing() const {
    return sealing_;
  }

  outcome::result<std::shared_ptr<MinerImpl>> MinerImpl::newMiner(
      std::shared_ptr<FullNodeApi> api,
      const Address &miner_address,
      const Address &worker_address,
      std::shared_ptr<Counter> counter,
      std::shared_ptr<BufferMap> sealing_fsm_kv,
      std::shared_ptr<Manager> sector_manager,
      const std::shared_ptr<Scheduler> &scheduler,
      std::shared_ptr<boost::asio::io_context> context,
      const mining::Config &config,
      const std::vector<Address>
          &precommit_control) {  // TODO(ortyomka): Commit Batcher extension
    // Checks miner worker address
    OUTCOME_TRY(key, api->StateAccountKey(worker_address, {}));
    OUTCOME_TRY(has, api->WalletHas(key));
    if (!has) {
      return MinerError::kWorkerNotFound;
    }

    // use empty TipsetKey
    OUTCOME_TRY(deadline_info,
                api->StateMinerProvingDeadline(miner_address, TipsetKey{}));

    std::shared_ptr<TipsetCache> tipset_cache =
        std::make_shared<TipsetCacheImpl>(2 * kGlobalChainConfidence, api);
    OUTCOME_TRY(events, EventsImpl::createEvents(api, tipset_cache));
    std::shared_ptr<PreCommitPolicy> precommit_policy =
        std::make_shared<BasicPreCommitPolicy>(
            api,
            deadline_info.period_start % kWPoStProvingPeriod,
            kMaxSectorExpirationExtension - 2 * kWPoStProvingPeriod);
    std::shared_ptr<FeeConfig> fee_config = std::make_shared<FeeConfig>();
    fee_config->max_precommit_batch_gas_fee.base = {
        0};  // TODO(ortyomka): config loading;
    fee_config->max_precommit_batch_gas_fee.per_sector =
        TokenAmount{"2000000000000000"};
    fee_config->max_precommit_gas_fee = TokenAmount{"25000000000000000"};
    std::shared_ptr<PreCommitBatcher> precommit_batcher =
        std::make_shared<PreCommitBatcherImpl>(
            std::chrono::seconds(60),
            api,
            miner_address,
            scheduler,
            [=](const MinerInfo &miner_info,
                const TokenAmount &good_funds,
                const std::shared_ptr<FullNodeApi> &api)
                -> outcome::result<Address> {
              return SelectAddress(miner_info, good_funds, api);
            },
            fee_config);
    OUTCOME_TRY(sealing,
                SealingImpl::newSealing(api,
                                        events,
                                        miner_address,
                                        std::move(counter),
                                        std::move(sealing_fsm_kv),
                                        std::move(sector_manager),
                                        precommit_policy,
                                        std::move(context),
                                        scheduler,
                                        precommit_batcher,
                                        config));

    struct make_unique_enabler : public MinerImpl {
      make_unique_enabler(std::shared_ptr<FullNodeApi> api,
                          std::shared_ptr<Sealing> sealing)
          : MinerImpl{std::move(api), std::move(sealing)} {};
    };

    std::shared_ptr<MinerImpl> miner =
        std::make_shared<make_unique_enabler>(api, sealing);

    return std::move(miner);
  }

}  // namespace fc::miner

OUTCOME_CPP_DEFINE_CATEGORY(fc::miner, MinerError, e) {
  using E = fc::miner::MinerError;

  if (e == E::kWorkerNotFound) {
    return "MinerError: key for worker not found in local wallet";
  }
  return "MinerError: unknown error";
}
