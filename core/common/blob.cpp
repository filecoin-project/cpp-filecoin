/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/common/blob.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, BlobError, e) {
  using fc::common::BlobError;

  switch (e) {
    case BlobError::INCORRECT_LENGTH:
      return "Input string has incorrect length, not matching the blob size";
  }

  return "Unknown error";
}

namespace fc::common {

  // explicit instantiations for the most frequently used blobs
  template class Blob<8ul>;
  template class Blob<16ul>;
  template class Blob<32ul>;
  template class Blob<64ul>;

}  // namespace filecoing::common
