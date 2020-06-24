/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify.hpp>

#include "api/rpc/ws.hpp"
#include "common/span.hpp"
#include "common/todo_error.hpp"
#include "node/builder.hpp"
#include "storage/car/car.hpp"
#include "storage/chain/chain_store.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "sync/hello.hpp"
#include "sync/peer_manager.hpp"
#include "vm/actor/builtin/init/init_actor.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

// TODO: move
template <>
struct fmt::formatter<libp2p::peer::PeerId> : formatter<std::string_view> {
  template <typename C>
  auto format(const libp2p::peer::PeerId &id, C &ctx) {
    auto str = id.toBase58();
    return formatter<std::string_view>::format(str, ctx);
  }
};

// TODO: move
template <>
struct fmt::formatter<libp2p::multi::Multiaddress>
    : formatter<std::string_view> {
  template <typename C>
  auto format(const libp2p::multi::Multiaddress &addr, C &ctx) {
    auto str = addr.getStringAddress();
    return formatter<std::string_view>::format(str, ctx);
  }
};

namespace fc::node {
  using primitives::block::BlockHeader;
  using storage::car::loadCar;

  auto readFile(const std::string &path) {
    std::ifstream file{path, std::ios::binary | std::ios::ate};
    if (!file.good()) {
      outcome::raise(TodoError::ERROR);
    }
    common::Buffer buffer;
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(common::span::string(buffer).data(), buffer.size());
    return buffer;
  }

  BlockHeader readGenesis(Config &config, gsl::span<const uint8_t> car) {
    auto ipld = std::make_shared<storage::ipfs::InMemoryDatastore>();
    OUTCOME_EXCEPT(roots, loadCar(*ipld, car));
    config.genesis_cid = roots[0];
    OUTCOME_EXCEPT(block, ipld->getCbor<BlockHeader>(config.genesis_cid));
    OUTCOME_EXCEPT(init_state,
                   vm::state::StateTreeImpl(ipld, block.parent_state_root)
                       .state<vm::actor::builtin::init::InitActorState>(
                           vm::actor::kInitAddress));
    config.network_name = init_state.network_name;
    return block;
  }

  void main(int argc, char **argv) {
    Config config;
    // TODO: config path
    config.init("", argc, argv);
    auto genesis_car = readFile(config.storage_car_file_name);
    auto genesis_block = readGenesis(config, genesis_car);

    OUTCOME_EXCEPT(objects, createNodeObjects(config));
    auto &io = *objects.io_context;
    auto &host = objects.host;
    OUTCOME_EXCEPT(loadCar(*objects.ipfs_datastore, genesis_car));

    OUTCOME_EXCEPT(objects.chain_store->writeGenesis(genesis_block));
    // TODO: feed cached heaviest tipset
    OUTCOME_EXCEPT(objects.chain_store->addBlock(genesis_block));

    auto peer_manager{std::make_shared<sync::PeerManager>(objects, config)};

    io.post([&] {
      OUTCOME_EXCEPT(host->listen(config.listen_address));
      for (auto &pi : config.bootstrap_list) {
        host->connect(pi);
      }

      host->start();
      objects.gossip->start();

      // TODO: graphsync start

      OUTCOME_EXCEPT(peer_manager->start());

      spdlog::info("Node started: /ip4/{}/tcp/{}/p2p/{}",
                   config.local_ip_address,
                   config.port,
                   host->getId().toBase58());

      // TODO: config port
      api::serve(*objects.api, io, "127.0.0.1", config.port + 1);
    });

    boost::asio::signal_set signals{io, SIGINT, SIGTERM};
    signals.async_wait([&](auto &, auto) { io.stop(); });
    io.run();
  }
}  // namespace fc::node

int main(int argc, char **argv) {
  fc::node::main(argc, argv);
}
