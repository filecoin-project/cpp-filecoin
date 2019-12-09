/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/filestore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::filestore, FileStoreError, e) {
  using fc::storage::filestore::FileStoreError;

  switch (e) {
    case (FileStoreError::FILE_NOT_FOUND):
      return "FileStore: file not found";
    case (FileStoreError::CANNOT_OPEN):
      return "FileStore: cannot open file";
    case (FileStoreError::FILE_CLOSED):
      return "FileStore: file closed";
    case (FileStoreError::DIRECTORY_NOT_FOUND):
      return "FileStore: directory not found";
    case (FileStoreError::NOT_DIRECTORY):
      return "FileStore: not a directory";
    default:
      return "FileStore: unknown error";
  }
}
