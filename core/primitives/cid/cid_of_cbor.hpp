/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CID_CID_OF_CBOR_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CID_CID_OF_CBOR_HPP

#include "primitives/cid/cid.hpp"
#include "codec/cbor/cbor.hpp"

namespace fc::primitives::cid {

  /**
   * @brief helper function to calculate cid of an object
   * @tparam T object type
   * @param value object instance reference
   * @return calculated cid or error
   */
  template <class T>
  outcome::result<CID> getCidOfCbor(const T &value) {
    OUTCOME_TRY(bytes, fc::codec::cbor::encode(value));
    return common::getCidOf(bytes);
  }
}  // namespace fc::primitives::cid

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CID_CID_OF_CBOR_HPP
