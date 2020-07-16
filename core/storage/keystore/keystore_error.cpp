/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/keystore/keystore_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::keystore, KeyStoreError, e) {
  using fc::storage::keystore::KeyStoreError;

  switch (e) {
    case KeyStoreError::kNotFound:
      return "KeyStoreError: address not found";
    case KeyStoreError::kAlreadyExists:
      return "KeyStoreError: address already exists";
    case KeyStoreError::kWrongAddress:
      return "KeyStoreError: wrong address";
    case KeyStoreError::kWrongSignature:
      return "KeyStoreError: wrong signature";
    case KeyStoreError::kCannotStore:
      return "KeyStoreError: cannot save key";
    case KeyStoreError::kCannotRead:
      return "KeyStoreError: cannot read key";
    default:
      return "unknown error";
  }
}
