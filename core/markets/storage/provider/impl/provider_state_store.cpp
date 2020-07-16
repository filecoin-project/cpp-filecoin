/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/impl/provider_state_store.hpp"

namespace fc::markets::storage::provider {
  ProviderFsmStateStore::ProviderFsmStateStore(std::shared_ptr<ProviderFSM> fsm)
      : fsm_{std::move(fsm)} {}

  outcome::result<MinerDeal> ProviderFsmStateStore::get(
      const CID &proposal_cid) const {
    for (const auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return *it.first;
      }
    }
    return ProviderStateStoreError::kStateNotFound;
  }

}  // namespace fc::markets::storage::provider

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::provider,
                            ProviderStateStoreError,
                            e) {
  using fc::markets::storage::provider::ProviderStateStoreError;

  switch (e) {
    case (ProviderStateStoreError::kStateNotFound):
      return "ProviderStateStoreError: state not found";
    default:
      return "ProviderStateStoreError: unknown error";
  }
}
