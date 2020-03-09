/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_BLOCKCHAIN_MESSAGE_POOL_ERROR
#define FILECOIN_CORE_BLOCKCHAIN_MESSAGE_POOL_ERROR

#include "filecoin/common/outcome.hpp"

namespace fc::blockchain::message_pool {

  /**
   * @brief Type of errors returned by MessagePool
   */
  enum class MessagePoolError {
    MESSAGE_ALREADY_IN_POOL = 1,
  };

}  // namespace fc::blockchain::message_pool

OUTCOME_HPP_DECLARE_ERROR(fc::blockchain::message_pool, MessagePoolError);

#endif  // FILECOIN_CORE_BLOCKCHAIN_MESSAGE_POOL_ERROR
