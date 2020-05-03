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
  }

}  // namespace fc::markets::storage::client
