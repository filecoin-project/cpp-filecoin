/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_MAKE_HPP
#define CPP_FILECOIN_CORE_API_MAKE_HPP

#include "api/api.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "common/todo_error.hpp"
#include "drand/beaconizer.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/interpreter/interpreter.hpp"

namespace fc::api {
  using blockchain::weight::WeightCalculator;
  using crypto::bls::BlsProvider;
  using drand::Beaconizer;
  using storage::blockchain::ChainStore;
  using storage::blockchain::MsgWaiter;
  using storage::keystore::KeyStore;
  using storage::mpool::Mpool;
  using vm::interpreter::Interpreter;
  using Logger = common::Logger;

  outcome::result<IpldObject> getNode(std::shared_ptr<Ipld> ipld,
                                      const CID &root,
                                      gsl::span<const std::string> parts);

  Api makeImpl(std::shared_ptr<ChainStore> chain_store,
               std::shared_ptr<WeightCalculator> weight_calculator,
               std::shared_ptr<Ipld> ipld,
               std::shared_ptr<BlsProvider> bls_provider,
               std::shared_ptr<Mpool> mpool,
               std::shared_ptr<Interpreter> interpreter,
               std::shared_ptr<MsgWaiter> msg_waiter,
               std::shared_ptr<Beaconizer> beaconizer,
               std::shared_ptr<KeyStore> key_store);
}  // namespace fc::api

#endif  // CPP_FILECOIN_CORE_API_MAKE_HPP
