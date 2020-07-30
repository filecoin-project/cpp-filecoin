/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_STATE_STORE_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_STATE_STORE_HPP

#include "fsm/fsm.hpp"
#include "fsm/state_store.hpp"
#include "markets/storage/client/client_events.hpp"
#include "markets/storage/deal_protocol.hpp"

namespace fc::markets::storage::client {
  using ClientFSM = fsm::FSM<ClientEvent, StorageDealStatus, ClientDeal>;
  using ClientStateStore = fsm::StateStore<CID, ClientDeal>;

  /**
   * Client state store implemented over client FSM
   */
  class ClientFsmStateStore : public ClientStateStore {
   public:
    explicit ClientFsmStateStore(std::shared_ptr<ClientFSM> fsm);

    outcome::result<ClientDeal> get(const CID &proposal_cid) const override;

   private:
    std::shared_ptr<ClientFSM> fsm_;
  };

  enum class ClientStateStoreError {
    kStateNotFound = 1,
  };

}  // namespace fc::markets::storage::client

OUTCOME_HPP_DECLARE_ERROR(fc::markets::storage::client, ClientStateStoreError);

#endif  // CPP_FILECOIN_MARKETS_STORAGE_CLIENT_CLIENT_STATE_STORE_HPP
