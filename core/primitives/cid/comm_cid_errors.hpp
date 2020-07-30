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
        kInvalidHash = 1
    };

}  // namespace fc::primitives::piece

OUTCOME_HPP_DECLARE_ERROR(fc::common, CommCidError);

#endif //CPP_FILECOIN_COMM_CID_ERRORS_HPP
