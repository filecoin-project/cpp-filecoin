/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/client/client_fsm_transitions.hpp"

namespace fc::markets::storage::client {

  void openStorageDealHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to) {
    deal->state = to;
    // TODO send event ClientEvent::ClientEventFundingInitiated
  }

  void initiateFundingHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to) {
    // TODO ensure client funds

    deal->state = to;
  }

  void ensureFundsFailHandler(std::shared_ptr<ClientDeal> deal,
                              ClientEvent event,
                              StorageDealStatus from,
                              StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void ensureFundsHandler(std::shared_ptr<ClientDeal> deal,
                          ClientEvent event,
                          StorageDealStatus from,
                          StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void writeProposalFailHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealProposedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void streamLookupErrorHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void readResponseFailHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void responseVerificationFailedHandler(std::shared_ptr<ClientDeal> deal,
                                         ClientEvent event,
                                         StorageDealStatus from,
                                         StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void responseDealDidNotMatchHandler(std::shared_ptr<ClientDeal> deal,
                                      ClientEvent event,
                                      StorageDealStatus from,
                                      StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealRejectedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealAcceptedHandler(std::shared_ptr<ClientDeal> deal,
                           ClientEvent event,
                           StorageDealStatus from,
                           StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void streamCloseErrorHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealPublishFailedHandler(std::shared_ptr<ClientDeal> deal,
                                ClientEvent event,
                                StorageDealStatus from,
                                StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealPublishedHandler(std::shared_ptr<ClientDeal> deal,
                            ClientEvent event,
                            StorageDealStatus from,
                            StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void activationFailedHandler(std::shared_ptr<ClientDeal> deal,
                               ClientEvent event,
                               StorageDealStatus from,
                               StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void dealActivationHandler(std::shared_ptr<ClientDeal> deal,
                             ClientEvent event,
                             StorageDealStatus from,
                             StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

  void eventFailedHandler(std::shared_ptr<ClientDeal> deal,
                          ClientEvent event,
                          StorageDealStatus from,
                          StorageDealStatus to) {
    // TODO
    deal->state = to;
  }

}  // namespace fc::markets::storage::client
