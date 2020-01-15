/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_ADDRESS_UTILS_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_ADDRESS_UTILS_HPP

#include "../../../../core/common/blob.hpp"
#include "../../../../core/common/hexutil.hpp"
#include "../../../../core/primitives/address/address.hpp"

/**
 * @brief creates id-type address
 * @param id address id
 * @return address object
 */
inline auto makeIdAddress(uint64_t id) {
  return fc::primitives::address::Address::makeFromId(id);
}

/**
 * @brief creates secp256k1-based address
 * @param data blob object
 * @return address object
 */
inline auto makeSecp256k1Address(const fc::common::Blob<20> &data) {
  return fc::primitives::address::Address{
      fc::primitives::address::Network::MAINNET,
      fc::primitives::address::Secp256k1PublicKeyHash{data}};
}

/**
 * @brief creates actorhash-based address
 * @param data blob object
 * @return address object
 */
inline auto makeActorHashAddress(const fc::common::Blob<20> &data) {
  return fc::primitives::address::Address{
      fc::primitives::address::Network::MAINNET,
      fc::primitives::address::ActorExecHash{data}};
}

/**
 * @brief creates bls-based address
 * @param data blob object
 * @return address object
 */
inline auto makeBlsAddress(const fc::common::Blob<48> &data) {
  return fc::primitives::address::Address{
      fc::primitives::address::Network::MAINNET,
      fc::primitives::address::BLSPublicKeyHash{data}};
}

#endif  // CPP_FILECOIN_TEST_TESTUTIL_PRIMITIVES_ADDRESS_UTILS_HPP
