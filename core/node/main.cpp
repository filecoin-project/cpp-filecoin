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

    std::shared_ptr<sync::Hello> hello = std::make_shared<sync::Hello>();

    io.post([&] {
      OUTCOME_EXCEPT(host->listen(config.listen_address));
      for (auto &pi : config.bootstrap_list) {
        host->connect(pi);
      }

      host->start();
      objects.gossip->start();

      // TODO: graphsync start

      hello->start(
          host,
          objects.utc_clock,
          config.genesis_cid,
          {{config.genesis_cid}, 0, genesis_block.parent_weight},
          [&](auto &peer, auto _s) {
            if (!_s) {
              spdlog::info("hello feedback failed for peer {}: {}",
                           peer,
                           _s.error().message());
              return;
            }
            auto &s = _s.value();
            spdlog::info(
                "hello feedback from peer {}: cids {}, height {}, weight {}",
                peer,
                fmt::join(s.heaviest_tipset, ","),
                s.heaviest_tipset_height,
                s.heaviest_tipset_weight.str());
          },
          [&](auto &peer, auto _latency) {
            if (!_latency) {
              spdlog::info("latency feedback failed for peer {}: {}",
                           peer,
                           _latency.error().message());
              return;
            }
            spdlog::info("latency feedback from peer {}: {} microsec",
                         peer,
                         _latency.value() / 1000);
          });

      auto handleProtocol = [&](auto &protocol) {
        host->setProtocolHandler(protocol.getProtocolId(), [&](auto res) {
          protocol.handle(std::move(res));
        });
      };
      handleProtocol(*objects.identify_protocol);
      handleProtocol(*objects.identify_push_protocol);
      handleProtocol(*objects.identify_delta_protocol);
      objects.identify_protocol->start();
      objects.identify_push_protocol->start();
      objects.identify_delta_protocol->start();

      auto identify_conn =
          objects.identify_protocol->onIdentifyReceived([&](auto &peer) {
            spdlog::info("Peer identify for {}:", peer);
            OUTCOME_EXCEPT(
                addresses,
                host->getPeerRepository().getAddressRepository().getAddresses(
                    peer));
            spdlog::info("  addresses: {}", fmt::join(addresses, " "));
            OUTCOME_EXCEPT(
                protocols,
                host->getPeerRepository().getProtocolRepository().getProtocols(
                    peer));
            spdlog::info("  protocols: {}", fmt::join(protocols, " "));
            hello->sayHello(peer);
          });

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
