/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_REPOSITORY_ERROR_HPP
#define FILECOIN_CORE_STORAGE_REPOSITORY_ERROR_HPP

#include "common/outcome.hpp"

namespace fc::storage::repository {

  /**
   * @brief Type of errors returned by Repository
   */
  enum class RepositoryError {
    kWrongVersion = 1,
    kOpenFileError,
  };

}  // namespace fc::storage::keystore

OUTCOME_HPP_DECLARE_ERROR(fc::storage::repository, RepositoryError);

#endif  // FILECOIN_CORE_STORAGE_REPOSITORY_ERROR_HPP
