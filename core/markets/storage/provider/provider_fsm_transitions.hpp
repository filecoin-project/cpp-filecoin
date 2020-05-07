/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_FSM_TRANSITIONS_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_FSM_TRANSITIONS_HPP

#include "fsm/fsm.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/provider/provider_events.hpp"

namespace fc::markets::storage::provider {

  using ProviderTransition =
      fsm::Transition<ProviderEvent, StorageDealStatus, MinerDeal>;

  /**
   * @brief Handle event open deal
   * @param deal  - current storage deal
   * @param event - ProviderEventOpen
   * @param from  - STORAGE_DEAL_UNKNOWN
   * @param to    - STORAGE_DEAL_VALIDATING
   */
  void eventOpenHandler(std::shared_ptr<MinerDeal> deal,
                        ProviderEvent event,
                        StorageDealStatus from,
                        StorageDealStatus to);

  /**
   * @brief Handle event node error
   * @param deal  - current storage deal
   * @param event - ProviderEventNodeErrored
   * @param from  - any
   * @param to    - STORAGE_DEAL_FAILING
   */
  void nodeErrorHandler(std::shared_ptr<MinerDeal> deal,
                        ProviderEvent event,
                        StorageDealStatus from,
                        StorageDealStatus to);

  /**
   * @brief Handle event deal rejected
   * @param deal  - current storage deal
   * @param event - ProviderEventOpen
   * @param from  - STORAGE_DEAL_VALIDATING or STORAGE_DEAL_VERIFY_DATA
   * @param to    - STORAGE_DEAL_FAILING
   */
  void dealRejectedHandler(std::shared_ptr<MinerDeal> deal,
                           ProviderEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle event deal accepted
   * @param deal  - current storage deal
   * @param event - ProviderEventDealAccepted
   * @param from  - STORAGE_DEAL_VALIDATING
   * @param to    - STORAGE_DEAL_PROPOSAL_ACCEPTED
   */
  void dealAcceptedHandler(std::shared_ptr<MinerDeal> deal,
                           ProviderEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle event waiting for manual data
   * @param deal  - current storage deal
   * @param event - ProviderEventWaitingForManualData
   * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
   * @param to    - STORAGE_DEAL_WAITING_FOR_DATA
   */
  void waitingForManualDataHandler(std::shared_ptr<MinerDeal> deal,
                                   ProviderEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

  /**
   * @brief Handle event data transfer failed
   * @param deal  - current storage deal
   * @param event - ProviderEventDataTransferFailed
   * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED or STORAGE_DEAL_TRANSFERRING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void dataTransferFailedHandler(std::shared_ptr<MinerDeal> deal,
                                 ProviderEvent event,
                                 StorageDealStatus from,
                                 StorageDealStatus to);

  /**
   * @brief Handle event data transfer initiated
   * @param deal  - current storage deal
   * @param event - ProviderEventDataTransferInitiated
   * @param from  - STORAGE_DEAL_PROPOSAL_ACCEPTED
   * @param to    - STORAGE_DEAL_TRANSFERRING
   */
  void dataTransferInitiatedHandler(std::shared_ptr<MinerDeal> deal,
                                    ProviderEvent event,
                                    StorageDealStatus from,
                                    StorageDealStatus to);

  /**
   * @brief Handle event data transfer completed
   * @param deal  - current storage deal
   * @param event - ProviderEventDataTransferCompleted
   * @param from  - STORAGE_DEAL_TRANSFERRING
   * @param to    - STORAGE_DEAL_VERIFY_DATA
   */
  void dataTransferCompletedHandler(std::shared_ptr<MinerDeal> deal,
                                    ProviderEvent event,
                                    StorageDealStatus from,
                                    StorageDealStatus to);

  /**
   * @brief Handle event generate piece CID failed
   * @param deal  - current storage deal
   * @param event - ProviderEventGeneratePieceCIDFailed
   * @param from  - STORAGE_DEAL_VERIFY_DATA
   * @param to    - STORAGE_DEAL_FAILING
   */
  void generatePieceCidFailedHandler(std::shared_ptr<MinerDeal> deal,
                                     ProviderEvent event,
                                     StorageDealStatus from,
                                     StorageDealStatus to);

  /**
   * @brief Handle event data verified
   * @param deal  - current storage deal
   * @param event - ProviderEventVerifiedData
   * @param from  - STORAGE_DEAL_VERIFY_DATA or STORAGE_DEAL_WAITING_FOR_DATA
   * @param to    - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
   */
  void verifiedDataHandler(std::shared_ptr<MinerDeal> deal,
                           ProviderEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle event funding initiated
   * @param deal  - current storage deal
   * @param event - ProviderEventFundingInitiated
   * @param from  - STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
   * @param to    - STORAGE_DEAL_PROVIDER_FUNDING
   */
  void fundingInitiatedHandler(std::shared_ptr<MinerDeal> deal,
                               ProviderEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

  /**
   * @brief Handle event funded
   * @param deal  - current storage deal
   * @param event - ProviderEventFunded
   * @param from  - STORAGE_DEAL_PROVIDER_FUNDING or
   * STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
   * @param to    - STORAGE_DEAL_PUBLISH
   */
  void fundedHandler(std::shared_ptr<MinerDeal> deal,
                     ProviderEvent event,
                     StorageDealStatus from,
                     StorageDealStatus to);

  /**
   * @brief Handle event publish initiated
   * @param deal  - current storage deal
   * @param event - ProviderEventDealPublishInitiated
   * @param from  - STORAGE_DEAL_PUBLISH
   * @param to    - STORAGE_DEAL_PUBLISHING
   */
  void publishInitiatedHandler(std::shared_ptr<MinerDeal> deal,
                               ProviderEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to);

  /**
   * @brief Handle event publish error
   * @param deal  - current storage deal
   * @param event - ProviderEventDealPublishError
   * @param from  - STORAGE_DEAL_PUBLISHING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void publishErrorHandler(std::shared_ptr<MinerDeal> deal,
                           ProviderEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to);

  /**
   * @brief Handle event send response failed
   * @param deal  - current storage deal
   * @param event - sendResponseFailedHandler
   * @param from  - STORAGE_DEAL_PUBLISHING or STORAGE_DEAL_FAILING
   * @param to    - STORAGE_DEAL_ERROR
   */
  void sendResponseFailedHandler(std::shared_ptr<MinerDeal> deal,
                                 ProviderEvent event,
                                 StorageDealStatus from,
                                 StorageDealStatus to);

  /**
   * @brief Handle event deal published
   * @param deal  - current storage deal
   * @param event - ProviderEventDealPublished
   * @param from  - STORAGE_DEAL_PUBLISHING
   * @param to    - STORAGE_DEAL_STAGED
   */
  void dealPublishedHandler(std::shared_ptr<MinerDeal> deal,
                            ProviderEvent event,
                            StorageDealStatus from,
                            StorageDealStatus to);

  /**
   * @brief Handle event file store error
   * @param deal  - current storage deal
   * @param event - ProviderEventFileStoreErrored
   * @param from  - STORAGE_DEAL_STAGED or STORAGE_DEAL_SEALING or
   * STORAGE_DEAL_ACTIVE
   * @param to    - STORAGE_DEAL_FAILING
   */
  void fileStoreErrorHandler(std::shared_ptr<MinerDeal> deal,
                             ProviderEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to);

  /**
   * @brief Handle event handoff failed
   * @param deal  - current storage deal
   * @param event - ProviderEventDealHandoffFailed
   * @param from  - STORAGE_DEAL_STAGED
   * @param to    - STORAGE_DEAL_FAILING
   */
  void dealHandoffFailedHandler(std::shared_ptr<MinerDeal> deal,
                                ProviderEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle event handoff
   * @param deal  - current storage deal
   * @param event - ProviderEventDealHandoffFailed
   * @param from  - STORAGE_DEAL_STAGED
   * @param to    - STORAGE_DEAL_SEALING
   */
  void dealHandoffHandler(std::shared_ptr<MinerDeal> deal,
                          ProviderEvent event,
                          StorageDealStatus from,
                          StorageDealStatus to);

  /**
   * @brief Handle event deal activation failed
   * @param deal  - current storage deal
   * @param event - ProviderEventDealActivationFailed
   * @param from  - STORAGE_DEAL_SEALING
   * @param to    - STORAGE_DEAL_FAILING
   */
  void dealActivationFailedHandler(std::shared_ptr<MinerDeal> deal,
                                   ProviderEvent event,
                                   StorageDealStatus from,
                                   StorageDealStatus to);

  /**
   * @brief Handle event deal activation
   * @param deal  - current storage deal
   * @param event - ProviderEventDealActivationFailed
   * @param from  - STORAGE_DEAL_SEALING
   * @param to    - STORAGE_DEAL_ACTIVE
   */
  void dealActivationHandler(std::shared_ptr<MinerDeal> deal,
                             ProviderEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to);

  /**
   * @brief Handle event piece store error
   * @param deal  - current storage deal
   * @param event - ProviderEventPieceStoreErrored
   * @param from  - STORAGE_DEAL_ACTIVE
   * @param to    - STORAGE_DEAL_FAILING
   */
  void pieceStoreErroredHandler(std::shared_ptr<MinerDeal> deal,
                                ProviderEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle event deal completed
   * @param deal  - current storage deal
   * @param event - ProviderEventDealCompleted
   * @param from  - STORAGE_DEAL_ACTIVE
   * @param to    - STORAGE_DEAL_COMPLETED
   */
  void dealCompletedHandler(std::shared_ptr<MinerDeal> deal,
                            ProviderEvent event,
                            StorageDealStatus from,
                            StorageDealStatus to);

  /**
   * @brief Handle event unable locate piece
   * @param deal  - current storage deal
   * @param event - ProviderEventUnableToLocatePiece
   * @param from  - STORAGE_DEAL_ACTIVE
   * @param to    - STORAGE_DEAL_FAILING
   */
  void unableLocatePieceHandler(std::shared_ptr<MinerDeal> deal,
                                ProviderEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to);

  /**
   * @brief Handle event read metadata error
   * @param deal  - current storage deal
   * @param event - ProviderEventReadMetadataErrored
   * @param from  - STORAGE_DEAL_ACTIVE
   * @param to    - STORAGE_DEAL_FAILING
   */
  void readMetadataErroredHandler(std::shared_ptr<MinerDeal> deal,
                                  ProviderEvent event,
                                  StorageDealStatus from,
                                  StorageDealStatus to);

  /**
   * @brief Handle event read metadata error
   * @param deal  - current storage deal
   * @param event - ProviderEventFailed
   * @param from  - STORAGE_DEAL_FAILING
   * @param to    - STORAGE_DEAL_ERROR
   */
  void failedHandler(std::shared_ptr<MinerDeal> deal,
                     ProviderEvent event,
                     StorageDealStatus from,
                     StorageDealStatus to);

  std::vector<ProviderTransition> provider_transitions = {
      ProviderTransition(ProviderEvent::ProviderEventOpen)
          .from(StorageDealStatus::STORAGE_DEAL_UNKNOWN)
          .to(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .action(eventOpenHandler),
      ProviderTransition(ProviderEvent::ProviderEventNodeErrored)
          .fromAny()
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(nodeErrorHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealRejected)
          .fromMany(StorageDealStatus::STORAGE_DEAL_VALIDATING,
                    StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(dealRejectedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealAccepted)
          .from(StorageDealStatus::STORAGE_DEAL_VALIDATING)
          .to(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .action(dealAcceptedHandler),
      ProviderTransition(ProviderEvent::ProviderEventWaitingForManualData)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA)
          .action(waitingForManualDataHandler),
      ProviderTransition(ProviderEvent::ProviderEventDataTransferFailed)
          .fromMany(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED,
                    StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(dataTransferFailedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDataTransferInitiated)
          .from(StorageDealStatus::STORAGE_DEAL_PROPOSAL_ACCEPTED)
          .to(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
          .action(dataTransferInitiatedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDataTransferCompleted)
          .from(StorageDealStatus::STORAGE_DEAL_TRANSFERRING)
          .to(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
          .action(dataTransferCompletedHandler),
      ProviderTransition(ProviderEvent::ProviderEventGeneratePieceCIDFailed)
          .from(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(generatePieceCidFailedHandler),
      ProviderTransition(ProviderEvent::ProviderEventVerifiedData)
          .fromMany(StorageDealStatus::STORAGE_DEAL_VERIFY_DATA,
                    StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA)
          .to(StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
          .action(verifiedDataHandler),
      ProviderTransition(ProviderEvent::ProviderEventFundingInitiated)
          .from(StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_PROVIDER_FUNDING)
          .action(fundingInitiatedHandler),
      ProviderTransition(ProviderEvent::ProviderEventFunded)
          .fromMany(StorageDealStatus::STORAGE_DEAL_PROVIDER_FUNDING,
                    StorageDealStatus::STORAGE_DEAL_ENSURE_PROVIDER_FUNDS)
          .to(StorageDealStatus::STORAGE_DEAL_PUBLISH)
          .action(fundedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealPublishInitiated)
          .from(StorageDealStatus::STORAGE_DEAL_PUBLISH)
          .to(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
          .action(publishInitiatedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealPublishError)
          .from(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(publishErrorHandler),
      ProviderTransition(ProviderEvent::ProviderEventSendResponseFailed)
          .fromMany(StorageDealStatus::STORAGE_DEAL_PUBLISHING,
                    StorageDealStatus::STORAGE_DEAL_FAILING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(sendResponseFailedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealPublished)
          .from(StorageDealStatus::STORAGE_DEAL_PUBLISHING)
          .to(StorageDealStatus::STORAGE_DEAL_STAGED)
          .action(dealPublishedHandler),
      ProviderTransition(ProviderEvent::ProviderEventFileStoreErrored)
          .fromMany(StorageDealStatus::STORAGE_DEAL_STAGED,
                    StorageDealStatus::STORAGE_DEAL_SEALING,
                    StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(fileStoreErrorHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealHandoffFailed)
          .from(StorageDealStatus::STORAGE_DEAL_STAGED)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(dealHandoffFailedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealHandedOff)
          .from(StorageDealStatus::STORAGE_DEAL_STAGED)
          .to(StorageDealStatus::STORAGE_DEAL_SEALING)
          .action(dealHandoffHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealActivationFailed)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(dealActivationFailedHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealActivated)
          .from(StorageDealStatus::STORAGE_DEAL_SEALING)
          .to(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .action(dealActivationHandler),
      ProviderTransition(ProviderEvent::ProviderEventPieceStoreErrored)
          .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(pieceStoreErroredHandler),
      ProviderTransition(ProviderEvent::ProviderEventDealCompleted)
          .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .to(StorageDealStatus::STORAGE_DEAL_COMPLETED)
          .action(dealCompletedHandler),
      ProviderTransition(ProviderEvent::ProviderEventUnableToLocatePiece)
          .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(unableLocatePieceHandler),
      ProviderTransition(ProviderEvent::ProviderEventReadMetadataErrored)
          .from(StorageDealStatus::STORAGE_DEAL_ACTIVE)
          .to(StorageDealStatus::STORAGE_DEAL_FAILING)
          .action(readMetadataErroredHandler),
      ProviderTransition(ProviderEvent::ProviderEventFailed)
          .from(StorageDealStatus::STORAGE_DEAL_FAILING)
          .to(StorageDealStatus::STORAGE_DEAL_ERROR)
          .action(failedHandler)};

}  // namespace fc::markets::storage::provider

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_PROVIDER_PROVIDER_FSM_TRANSITIONS_HPP
