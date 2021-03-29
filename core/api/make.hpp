/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/node_api.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "fwd.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::api {
  using blockchain::weight::WeightCalculator;
  using crypto::bls::BlsProvider;
  using drand::Beaconizer;
  using drand::DrandSchedule;
  using storage::blockchain::ChainStore;
  using storage::blockchain::MsgWaiter;
  using storage::keystore::KeyStore;
  using storage::mpool::MessagePool;
  using sync::PubSubGate;
  using vm::runtime::EnvironmentContext;
  using Logger = common::Logger;

  outcome::result<IpldObject> getNode(std::shared_ptr<Ipld> ipld,
                                      const CID &root,
                                      gsl::span<const std::string> parts);

  std::shared_ptr<FullNodeApi> makeImpl(
      std::shared_ptr<ChainStore> chain_store,
      const std::string &network_name,
      std::shared_ptr<WeightCalculator> weight_calculator,
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      std::shared_ptr<MessagePool> mpool,
      std::shared_ptr<MsgWaiter> msg_waiter,
      std::shared_ptr<Beaconizer> beaconizer,
      std::shared_ptr<DrandSchedule> drand_schedule,
      std::shared_ptr<PubSubGate> pubsub,
      std::shared_ptr<KeyStore> key_store);
}  // namespace fc::api
