/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_MAKE_HPP
#define CPP_FILECOIN_CORE_API_MAKE_HPP

#include "api/api.hpp"
#include "blockchain/weight_calculator.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/ipfs/datastore.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::api {
  using blockchain::weight::WeightCalculator;
  using storage::blockchain::ChainStore;
  using storage::ipfs::IpfsDatastore;
  using storage::keystore::KeyStore;

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<IpfsDatastore> ipld,
               std::shared_ptr<KeyStore> key_store);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_MAKE_HPP
