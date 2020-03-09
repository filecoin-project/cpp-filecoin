/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_CORE_STORAGE_FILESTORE_PATH_HPP
#define FILECOIN_CORE_STORAGE_FILESTORE_PATH_HPP

#include <string>

namespace fc::storage::filestore {

  /** Directory separator character */
  const char DELIMITER = '/';

  using Path = std::string;

}  // namespace fc::storage::filestore

#endif  // FILECOIN_CORE_STORAGE_FILESTORE_PATH_HPP
