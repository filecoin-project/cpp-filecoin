/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_COMM_CID_ERRORS_HPP
#define CPP_FILECOIN_COMM_CID_ERRORS_HPP


#include "common/outcome.hpp"

namespace fc::common {

    /**
     * @brief Pieces returns these types of errors
     */
    enum class CommCidError {
        TOO_SHORT = 1,
        CANT_READ_CODE,
        CANT_READ_LENGTH,
        DATA_TOO_BIG,
        DIFFERENT_LENGTH,
        INVALID_HASH
    };

}  // namespace fc::primitives::piece

OUTCOME_HPP_DECLARE_ERROR(fc::common, CommCidError);

#endif //CPP_FILECOIN_COMM_CID_ERRORS_HPP
