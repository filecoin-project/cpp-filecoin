/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_PROVIDER_HPP

#include "primitives/types.hpp"

namespace fc::markets::retrieval::provider {
  using primitives::TokenAmount;

  /**
   * @class Retrieval market provider
   */
  class RetrievalProvider {
   public:
    /**
     * @brief Destructor
     */
    virtual ~RetrievalProvider() = default;

    /**
     * @brief Start listening for new deals on the given host
     * @return operation result
     */
    virtual void start() = 0;

    /**
     * @brief Set price per sent byte
     * @param amount - number of tokens
     */
    virtual void setPricePerByte(TokenAmount amount) = 0;

    /**
     * @brief Set number of bytes before receive next payment
     * @param payment_interval - number of bytes before next payment
     * @param payment_interval_increase - increase rate
     */
    virtual void setPaymentInterval(uint64_t payment_interval,
                                    uint64_t payment_interval_increase) = 0;
  };

}  // namespace fc::markets::retrieval::provider

#endif
