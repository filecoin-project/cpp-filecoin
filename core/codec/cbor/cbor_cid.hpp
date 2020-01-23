/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_CID_HPP
#define CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_CID_HPP

#include "codec/cbor/cbor.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::codec::cbor {

  /**
   * @brief helper function to calculate cid of an object
   * @tparam T object type
   * @param value object instance reference
   * @return calculated cid or error
   */
  template <class T>
  outcome::result<CID> getCidOfCbor(const T &value) {
    OUTCOME_TRY(bytes, encode(value));
    return common::getCidOf(bytes);
  }
}  // namespace fc::codec::cbor

#endif  // CPP_FILECOIN_CORE_CODEC_CBOR_CBOR_CID_HPP
