/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_LEVELDB_ERROR_HPP
#define CPP_FILECOIN_LEVELDB_ERROR_HPP

#include <outcome/outcome.hpp>

namespace fc::storage {

  /**
   * @brief LevelDB returns those type of errors, as described in
   * <leveldb/status.h>, Status::Code (it is private)
   */
  enum class LevelDBError {
    OK = 0,
    NOT_FOUND = 1,
    CORRUPTION = 2,
    NOT_SUPPORTED = 3,
    INVALID_ARGUMENT = 4,
    IO_ERROR = 5,

    UNKNOWN = 1000
  };

}  // namespace fc::storage

OUTCOME_HPP_DECLARE_ERROR(fc::storage, LevelDBError);

#endif  // CPP_FILECOIN_LEVELDB_ERROR_HPP
