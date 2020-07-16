/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/filestore/filestore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::filestore, FileStoreError, e) {
  using fc::storage::filestore::FileStoreError;

  switch (e) {
    case (FileStoreError::kFileNotFound):
      return "FileStore: file not found";
    case (FileStoreError::kCannotOpen):
      return "FileStore: cannot open file";
    case (FileStoreError::kFileClosed):
      return "FileStore: file closed";
    case (FileStoreError::kDirectoryNotFound):
      return "FileStore: directory not found";
    case (FileStoreError::kNotDirectory):
      return "FileStore: not a directory";
    default:
      return "FileStore: unknown error";
  }
}
