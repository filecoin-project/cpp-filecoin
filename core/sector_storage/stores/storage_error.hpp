/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"

namespace fc::sector_storage::stores {

  enum class StorageError {
    kFileNotExist = 1,
    kFilesystemStatError,
    kConfigFileNotExist,
  };

}

OUTCOME_HPP_DECLARE_ERROR(fc::sector_storage::stores, StorageError);
