/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_FSM_TRANSITIONS_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_FSM_TRANSITIONS_HPP

#include "fsm/fsm.hpp"
#include "markets/storage/client/client_events.hpp"
#include "markets/storage/deal_protocol.hpp"

namespace fc::markets::storage::client {

  using ClientTransition =
      fsm::Transition<ClientEvent, StorageDealStatus, ClientDeal>;

  /**
   * @brief Handle open storage deal event
   * @param deal  - current storage deal
   * @param event - ClientEventOpen
   * @param from  - STORAGE_DEAL_UNKNOWN
   * @param to    - STORAGE_DEAL_ENSURE_CLIENT_FUNDS
   */
  void openStorageDealHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to);

  std::vector<ClientTransition> client_transitions = {
      ClientTransition(ClientEvent::ClientEventOpen)
          .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
          .to(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .action(openStorageDealHandler),
      ClientTransition(ClientEvent::ClientEventFundingInitiated)
          .from(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING),
      ClientTransition(ClientEvent::ClientEventEnsureFundsFailed)
          .fromMany(StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING,
                    StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING),
      ClientTransition(ClientEvent::ClientEventFundsEnsured)
          .fromMany(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS,
                    StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING)
          .to(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED),
      ClientTransition(ClientEvent::ClientEventWriteProposalFailed)
          .from(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR),
      ClientTransition(ClientEvent::ClientEventDealProposed)
          .from(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
          .to(StorageDealStatus::STORAGE_DEAL_VALIDATING),
      ClientTransition(ClientEvent::ClientEventDealStreamLookupErrored)
          .fromAny()
          .to(StorageDealStatus::STORAGE_DEAL_FAILING),
      ClientTransition(ClientEvent::ClientEventReadResponseFailed)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR),
      ClientTransition(ClientEvent::ClientEventResponseVerificationFailed)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING),
      ClientTransition(ClientEvent::ClientEventResponseDealDidNotMatch)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING),
      ClientTransition(ClientEvent::ClientEventDealRejected)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING),
      ClientTransition(ClientEvent::ClientEventDealAccepted)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED),
      ClientTransition(ClientEvent::ClientEventStreamCloseError)
          .fromAny()
          .to(StorageDealStatus::STORAGE_DEAL_ERROR),
      ClientTransition(ClientEvent::ClientEventDealPublishFailed)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR),
      ClientTransition(ClientEvent::ClientEventDealPublished)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_SEALING),
      ClientTransition(ClientEvent::ClientEventDealActivationFailed)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR),
      ClientTransition(ClientEvent::ClientEventDealActivated)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_ACTIVE),
      ClientTransition(ClientEvent::ClientEventFailed)
          .from(StorageDealStatus::STORAGE_DEAL_FAILING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)};

}  // namespace fc::markets::storage::client

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_FSM_TRANSITIONS_HPP
