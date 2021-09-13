/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/types.hpp"

namespace fc::markets::retrieval::provider {
  using primitives::TokenAmount;

  inline const TokenAmount kDefaultPricePerByte{2};
  inline const TokenAmount kDefaultUnsealPrice{0};
  constexpr uint64_t kDefaultPaymentInterval{1 << 20};
  constexpr uint64_t kDefaultPaymentIntervalIncrease{1 << 20};

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

    virtual RetrievalAsk getAsk() const = 0;

    virtual void setAsk(const RetrievalAsk &ask) = 0;

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
