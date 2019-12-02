/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_ERROR_HPP
#define FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::filestore {

  /**
   * @brief FileStore returns these types of errors
   */
  enum class FileStoreError {
    FILE_NOT_FOUND = 1,
    CANNOT_OPEN = 2,
    FILE_CLOSED = 3,

    UNKNOWN = 1000
  };

}  // namespace fc::storage::filestore

OUTCOME_HPP_DECLARE_ERROR(fc::storage::filestore, FileStoreError);

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_FILESTORE_ERROR_HPP
