/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_STATE_STORE_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_STATE_STORE_HPP

#include "fsm/fsm.hpp"
#include "fsm/state_store.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/provider/provider_events.hpp"

namespace fc::markets::storage::provider {
  using ProviderFSM = fsm::FSM<ProviderEvent, StorageDealStatus, MinerDeal>;
  using ProviderStateStore = fsm::StateStore<CID, MinerDeal>;

  /**
   * Provider state store implemented over provider FSM
   */
  class ProviderFsmStateStore : public ProviderStateStore {
   public:
    explicit ProviderFsmStateStore(std::shared_ptr<ProviderFSM> fsm);

    outcome::result<MinerDeal> get(const CID &proposal_cid) const override;

   private:
    std::shared_ptr<ProviderFSM> fsm_;
  };

  enum class ProviderStateStoreError { kStateNotFound = 1 };

}  // namespace fc::markets::storage::provider

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::provider,
                          ProviderStateStoreError);

#endif  // CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_STATE_STORE_HPP
