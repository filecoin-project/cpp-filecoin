/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::storage::ipfs {

  enum class ApiIpfsDatastoreError {
    kNotSupproted = 1,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs, ApiIpfsDatastoreError);
