/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/keystore/keystore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::keystore, KeyStoreError, e) {
  using fc::storage::keystore::KeyStoreError;

  switch (e) {
    case KeyStoreError::NOT_FOUND:
      return "KeyStoreError: address not found";
    case KeyStoreError::ALREADY_EXISTS:
      return "KeyStoreError: address already exists";
    case KeyStoreError::WRONG_ADDRESS:
      return "KeyStoreError: wrong address";
    case KeyStoreError::WRONG_SIGNATURE:
      return "KeyStoreError: wrong signature";
    case KeyStoreError::CANNOT_STORE:
      return "KeyStoreError: cannot save key";
    case KeyStoreError::CANNOT_READ:
      return "KeyStoreError: cannot read key";
    case KeyStoreError::UNKNOWN:
      break;
  }

  return "unknown error";
}
