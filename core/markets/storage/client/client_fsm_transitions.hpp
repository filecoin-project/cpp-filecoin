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

  /**
   * @brief Handle initiate funding
   * @param deal  - current storage deal
   * @param event - ClientEventFundingInitiated
   * @param from  - STORAGE_DEAL_ENSURE_CLIENT_FUNDS
   * @param to    - STORAGE_DEAL_CLIENT_FUNDING
   */
  void initiateFundingHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to);

  /**
   * @brief Handle ensure funds fail
   * @param deal  - current storage deal
   * @param event - ClientEventEnsureFundsFailed
   * @param from  - STORAGE_DEAL_CLIENT_FUNDING or
   * STORAGE_DEAL_ENSURE_CLIENT_FUNDS
   * @param to    - STORAGE_DEAL_FAILING
   */
  void ensureFundsFailHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to);

  /**
   * @brief Handle ensure funds
   * @param deal  - current storage deal
   * @param event - ClientEventEnsureFundsFailed
   * @param from  - STORAGE_DEAL_ENSURE_CLIENT_FUNDS or
   * STORAGE_DEAL_CLIENT_FUNDING
   * @param to    - STORAGE_DEAL_FUNDS_ENSURED
   */
  void ensureFundsHandler(std::shared_ptr<ClientDeal> deal,
                          ClientEvent event,
                          StorageDealStatus from,
                          StorageDealStatus to);

  /**
   * @brief Handle write proposal fail
   * @param deal  - current storage deal
   * @param event - ClientEventEnsureFundsFailed
   * @param from  - STORAGE_DEAL_FUNDS_ENSURED
   * @param to    - STORAGE_DEAL_ERROR
   */
  void writeProposalFailHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle deal proposal
   * @param deal  - current storage deal
   * @param event - ClientEventEnsureFundsFailed
   * @param from  - STORAGE_DEAL_FUNDS_ENSURED
   * @param to    - STORAGE_DEAL_VALIDATING
   */
  void dealProposedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle deal stream lookup error
   * @param deal  - current storage deal
   * @param event - ClientEventDealStreamLookupErrored
   * @param from  - any state
   * @param to    - STORAGE_DEAL_FAILING
   */
  void streamLookupErrorHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle read response fail
   * @param deal  - current storage deal
   * @param event - ClientEventReadResponseFailed
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_ERROR
   */
  void readResponseFailHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

  /**
   * @brief Handle response verification fail
   * @param deal  - current storage deal
   * @param event - ClientEventResponseVerificationFailed
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void responseVerificationFailedHandler(std::shared_ptr<ClientDeal> deal,
                                         ClientEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to);

  /**
   * @brief Handle response deal didn't match
   * @param deal  - current storage deal
   * @param event - ClientEventResponseDealDidNotMatch
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void responseDealDidNotMatchHandler(std::shared_ptr<ClientDeal> deal,
                                      ClientEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to);

  /**
   * @brief Handle deal reject
   * @param deal  - current storage deal
   * @param event - ClientEventDealRejected
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void dealRejectedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle deal accepted
   * @param deal  - current storage deal
   * @param event - ClientEventDealAccepted
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_PROPOSAL_ACCEPTED
   */
  void dealAcceptedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle stream close error
   * @param deal  - current storage deal
   * @param event - ClientEventStreamCloseError
   * @param from  - any
   * @param to    - STORAGE_DEAL_ERROR
   */
  void streamCloseErrorHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

  /**
   * @brief Handle deal publish fail
   * @param deal  - current storage deal
   * @param event - ClientEventDealPublishFailed
   * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
   * @param to    - STORAGE_DEAL_ERROR
   */
  void dealPublishFailedHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle deal published
   * @param deal  - current storage deal
   * @param event - ClientEventDealPublished
   * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
   * @param to    - STORAGE_DEAL_SEALING
   */
  void dealPublishedHandler(std::shared_ptr<ClientDeal> deal,
                            ClientEvent event,
                            StorageDealStatus from,
                            StorageDealStatus to);
  /**
   * @brief Handle activation fail
   * @param deal  - current storage deal
   * @param event - ClientEventDealActivationFailed
   * @param from  - STORAGE_DEAL_SEALING
   * @param to    - STORAGE_DEAL_ERROR
   */
  void activationFailedHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

  /**
   * @brief Handle deal activation
   * @param deal  - current storage deal
   * @param event - ClientEventDealActivated
   * @param from  - STORAGE_DEAL_SEALING
   * @param to    - STORAGE_DEAL_ACTIVE
   */
  void dealActivationHandler(std::shared_ptr<ClientDeal> deal,
                             ClientEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to);

  /**
   * @brief Handle event fail
   * @param deal  - current storage deal
   * @param event - ClientEventFailed
   * @param from  - STORAGE_DEAL_FAILING
   * @param to    - STORAGE_DEAL_ERROR
   */
  void eventFailedHandler(std::shared_ptr<ClientDeal> deal,
                          ClientEvent event,
                          StorageDealStatus from,
                          StorageDealStatus to);

  static std::vector<ClientTransition> client_transitions = {
      ClientTransition(ClientEvent::ClientEventOpen)
          .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
          .to(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .action(openStorageDealHandler),
      ClientTransition(ClientEvent::ClientEventFundingInitiated)
          .from(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING)
          .action(initiateFundingHandler),
      ClientTransition(ClientEvent::ClientEventEnsureFundsFailed)
          .fromMany(StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING,
                    StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(ensureFundsFailHandler),
      ClientTransition(ClientEvent::ClientEventFundsEnsured)
          .fromMany(StorageDealStatus::STORAGE_DEAL_ENSURE_CLIENT_FUNDS,
                    StorageDealStatus::STORAGE_DEAL_CLIENT_FUNDING)
          .to(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
          .action(ensureFundsHandler),
      ClientTransition(ClientEvent::ClientEventWriteProposalFailed)
          .from(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(writeProposalFailHandler),
      ClientTransition(ClientEvent::ClientEventDealProposed)
          .from(StorageDealStatus::STORAGE_DEAL_FUNDS_ENSURED)
          .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .action(dealProposedHandler),
      ClientTransition(ClientEvent::ClientEventDealStreamLookupErrored)
          .fromAny()
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(streamLookupErrorHandler),
      ClientTransition(ClientEvent::ClientEventReadResponseFailed)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(readResponseFailHandler),
      ClientTransition(ClientEvent::ClientEventResponseVerificationFailed)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(responseVerificationFailedHandler),
      ClientTransition(ClientEvent::ClientEventResponseDealDidNotMatch)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(responseDealDidNotMatchHandler),
      ClientTransition(ClientEvent::ClientEventDealRejected)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(dealRejectedHandler),
      ClientTransition(ClientEvent::ClientEventDealAccepted)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .action(dealAcceptedHandler),
      ClientTransition(ClientEvent::ClientEventStreamCloseError)
          .fromAny()
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(streamCloseErrorHandler),
      ClientTransition(ClientEvent::ClientEventDealPublishFailed)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(dealPublishFailedHandler),
      ClientTransition(ClientEvent::ClientEventDealPublished)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_SEALING)
          .action(dealPublishedHandler),
      ClientTransition(ClientEvent::ClientEventDealActivationFailed)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(activationFailedHandler),
      ClientTransition(ClientEvent::ClientEventDealActivated)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .action(dealActivationHandler),
      ClientTransition(ClientEvent::ClientEventFailed)
          .from(StorageDealStatus::STORAGE_DEAL_FAILING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(eventFailedHandler)};

}  // namespace fc::markets::storage::client

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_CLIENT_FSM_TRANSITIONS_HPP
