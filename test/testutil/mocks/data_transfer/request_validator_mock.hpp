/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESUTIL_MOCKS_DATA_TRANSFER_REQUEST_VALIDATOR_MOCK_HPP
#define CPP_FILECOIN_TESUTIL_MOCKS_DATA_TRANSFER_REQUEST_VALIDATOR_MOCK_HPP

#include "data_transfer/request_validator.hpp"

#include <gmock/gmock.h>

namespace fc::data_transfer {

  class RequestValidatorMock : public RequestValidator {
   public:
    MOCK_METHOD4(validatePush,
                 outcome::result<void>(const PeerInfo &sender,
                                       std::vector<uint8_t> voucher,
                                       CID base_cid,
                                       std::shared_ptr<Selector> selector));

    MOCK_METHOD4(validatePull,
                 outcome::result<void>(const PeerInfo &receiver,
                                       std::vector<uint8_t> voucher,
                                       CID base_cid,
                                       std::shared_ptr<Selector> selector));
  };

}  // namespace fc::data_transfer

#endif  // CPP_FILECOIN_TESUTIL_MOCKS_DATA_TRANSFER_REQUEST_VALIDATOR_MOCK_HPP
