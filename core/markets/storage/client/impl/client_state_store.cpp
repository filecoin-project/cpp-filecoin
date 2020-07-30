/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/impl/client_state_store.hpp"

namespace fc::markets::storage::client {
  ClientFsmStateStore::ClientFsmStateStore(std::shared_ptr<ClientFSM> fsm)
      : fsm_{std::move(fsm)} {}

  outcome::result<ClientDeal> ClientFsmStateStore::get(
      const CID &proposal_cid) const {
    for (const auto &it : fsm_->list()) {
      if (it.first->proposal_cid == proposal_cid) {
        return *it.first;
      }
    }
    return ClientStateStoreError::kStateNotFound;
  }

}  // namespace fc::markets::storage::client

OUTCOME_CPP_DEFINE_CATEGORY(fc::markets::storage::client,
                            ClientStateStoreError,
                            e) {
  using fc::markets::storage::client::ClientStateStoreError;

  switch (e) {
    case (ClientStateStoreError::kStateNotFound):
      return "ClientStateStoreError: state not found";
    default:
      return "ClientStateStoreError: unknown error";
  }
}
