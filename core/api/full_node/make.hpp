/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "api/wallet/wallet_api.hpp"
#include "blockchain/weight_calculator.hpp"
#include "common/logger.hpp"
#include "fwd.hpp"
#include "markets/discovery/discovery.hpp"
#include "markets/retrieval/client/retrieval_client.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/chain/msg_waiter.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/mpool/mpool.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::api {
  using blockchain::weight::WeightCalculator;
  using crypto::bls::BlsProvider;
  using drand::Beaconizer;
  using drand::DrandSchedule;
  using markets::discovery::Discovery;
  using markets::retrieval::client::RetrievalClient;
  using storage::OneKey;
  using storage::blockchain::ChainStore;
  using storage::blockchain::MsgWaiter;
  using storage::keystore::KeyStore;
  using storage::mpool::MessagePool;
  using sync::PubSubGate;
  using vm::runtime::EnvironmentContext;

  const static common::Logger kNodeApiLogger =
      common::createLogger("Full Node API");

  outcome::result<IpldObject> getNode(const std::shared_ptr<Ipld> &ipld,
                                      const CID &root,
                                      gsl::span<const std::string> parts);

  std::shared_ptr<FullNodeApi> makeImpl(
      std::shared_ptr<FullNodeApi> api,
      const std::shared_ptr<ChainStore> &chain_store,
      const IpldPtr &markets_ipld,
      const std::string &network_name,
      const std::shared_ptr<WeightCalculator> &weight_calculator,
      const EnvironmentContext &env_context,
      TsBranchPtr ts_main,
      const std::shared_ptr<MessagePool> &mpool,
      const std::shared_ptr<MsgWaiter> &msg_waiter,
      const std::shared_ptr<Beaconizer> &beaconizer,
      const std::shared_ptr<DrandSchedule> &drand_schedule,
      const std::shared_ptr<PubSubGate> &pubsub,
      const std::shared_ptr<KeyStore> &key_store,
      const std::shared_ptr<Discovery> &market_discovery,
      const std::shared_ptr<RetrievalClient> &retrieval_market_client,
      const std::shared_ptr<WalletApi> &wallet);
}  // namespace fc::api
