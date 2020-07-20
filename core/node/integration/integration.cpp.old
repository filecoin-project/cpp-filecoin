/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>

#include <spdlog/fmt/fmt.h>

#include <boost/program_options.hpp>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify.hpp>

#include "node/builder.hpp"

#include "common/logger.hpp"
#include "storage/car/car.hpp"

#include "sync/hello.hpp"

#include "primitives/cid/cid_of_cbor.hpp"

std::ostream &operator<<(std::ostream &os, const fc::CID &cid) {
  os << cid.toString().value();
  return os;
}

namespace {
  auto log() {
    static fc::common::Logger logger = fc::common::createLogger("sync");
    return logger.get();
  }

  void onPeerIdentify(libp2p::Host &host,
                      fc::sync::Hello &hello,
                      const libp2p::peer::PeerId &peer) {
    std::cerr << "Peer identify for " << peer.toBase58() << ":\n";

    auto addr_res =
        host.getPeerRepository().getAddressRepository().getAddresses(peer);

    if (addr_res) {
      std::cerr << "addresses: ";
      for (const auto &addr : addr_res.value()) {
        std::cerr << addr.getStringAddress() << ' ';
      }
      std::cerr << '\n';
    } else {
      std::cerr << addr_res.error().message() << '\n';
    }

    auto proto_res =
        host.getPeerRepository().getProtocolRepository().getProtocols(peer);

    if (proto_res) {
      std::cerr << "protocols: ";
      for (const auto &proto : proto_res.value()) {
        std::cerr << proto << ' ';
      }
      std::cerr << '\n';
    } else {
      std::cerr << proto_res.error().message() << '\n';
    }

    hello.sayHello(peer);
  }

  void handleProtocol(libp2p::Host &host,
                      libp2p::protocol::BaseProtocol &protocol) {
    host.setProtocolHandler(
        protocol.getProtocolId(),
        [&protocol](libp2p::protocol::BaseProtocol::StreamResult res) {
          protocol.handle(std::move(res));
        });
  }

  gsl::span<const uint8_t> toSpanU8(const std::string &s) {
    return gsl::span<const uint8_t>((const uint8_t *)s.data(), s.size());
  }

  std::vector<std::string> toStrings(const std::vector<fc::CID> &cids) {
    std::vector<std::string> v;
    v.reserve(cids.size());
    for (const auto &cid : cids) {
      v.push_back(cid.toString().value());
    }
    return v;
  }

  bool loadCar(const std::string &file_name,
               fc::storage::ipfs::IpfsDatastore &storage,
               const fc::CID &expected_genesis) {

    // if genesis in storage then all is ok

    std::ifstream file{file_name, std::ios::binary | std::ios::ate};
    if (!file.good()) {
      log()->error("cannot open file {}", file_name);
      return false;
    }

    static const size_t kMaxSize = 1024 * 1024;
    auto size = static_cast<size_t>(file.tellg());
    if (size > kMaxSize) {
      log()->error(
          "file size above expected, file:{}, size:{}", file_name, size);
      return false;
    }

    std::string buffer;
    buffer.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), buffer.size());

    auto result = fc::storage::car::loadCar(storage, toSpanU8(buffer));
    if (!result) {
      log()->error(
          "cannot load car file {}: {}", file_name, result.error().message());
      return false;
    }

    auto contains = storage.contains(expected_genesis);
    if (!contains) {
      log()->error("cannot load expected genesis cid:{} from car file:{}, {}",
                   expected_genesis.toString().value(),
                   file_name,
                   contains.error().message());
      return false;
    }

    if (!contains.value()) {
      log()->error(
          "cannot load expected genesis cid:{} from car file:{} which "
          "contains: {}",
          expected_genesis.toString().value(),
          file_name,
          fmt::join(toStrings(result.value()), " "));
      return false;
    }

    log()->debug("car file {} contains {}",
                 file_name,
                 fmt::join(toStrings(result.value()), " "));
    return true;
  }

  void helloFeedback(
      const libp2p::peer::PeerId &peer,
      fc::outcome::result<fc::sync::Hello::Message> state) {
    if (!state) {
      log()->info("hello feedback failed for peer {}: {}",
                  peer.toBase58(),
                  state.error().message());
      return;
    }

    auto &s = state.value();

    log()->info("hello feedback from peer:{}, cids:{}, height:{}, weight:{}",
                peer.toBase58(),
                fmt::join(toStrings(s.heaviest_tipset), ","),
                s.heaviest_tipset_height,
                s.heaviest_tipset_weight.str());
  }

  void latencyFeedback(const libp2p::peer::PeerId &peer,
                       fc::outcome::result<uint64_t> result) {
    if (!result) {
      log()->info("latency feedback failed for peer {}: {}",
                  peer.toBase58(),
                  result.error().message());
      return;
    }

    log()->info("latency feedback from peer {}: {} microsec",
                peer.toBase58(),
                result.value() / 1000);
  }

}  // namespace

int main(int argc, char *argv[]) {
  fc::node::Config config;

  if (!config.init("", argc, argv)) {
    return 1;
  }

  auto res = fc::node::createNodeObjects(config);

  if (!res) {
    std::cerr << "cannot create objects:" << res.error().message() << "\n";
    return 2;
  }

  auto &objects = res.value();

  auto io = objects.io_context;
  auto host = objects.host;

  loadCar(config.storage_car_file_name,
          *objects.ipfs_datastore,
          config.genesis_cid);

  std::shared_ptr<fc::sync::Hello> hello = std::make_shared<fc::sync::Hello>();

  auto identify_conn = objects.identify_protocol->onIdentifyReceived(
      [&host, &hello](const libp2p::peer::PeerId &peer) {
        onPeerIdentify(*host, *hello, peer);
      });

  // start the node as soon as async engine starts
  io->post([&] {
    auto listen_res = host->listen(config.listen_address);
    if (!listen_res) {
      std::cerr << "Cannot listen to multiaddress "
                << config.listen_address.getStringAddress() << ", "
                << listen_res.error().message() << "\n";
      io->stop();
      return;
    }

    handleProtocol(*host, *objects.identify_protocol);
    handleProtocol(*host, *objects.identify_push_protocol);
    handleProtocol(*host, *objects.identify_delta_protocol);

    host->start();

    objects.identify_protocol->start();
    objects.identify_push_protocol->start();
    objects.identify_delta_protocol->start();

    fc::sync::Hello::Message initial_state;
    initial_state.heaviest_tipset = {config.genesis_cid};
    initial_state.heaviest_tipset_height = 0;
    initial_state.heaviest_tipset_weight =
        fc::primitives::BigInt("100500100500100500");

    hello->start(host,
                 objects.utc_clock,
                 config.genesis_cid,
                 initial_state,
                 &helloFeedback,
                 &latencyFeedback);

    for (const auto &pi : config.bootstrap_list) {
      host->connect(pi);
    }

    // gossip->start();
    std::cerr << "Node started: "
              << fmt::format("/ip4/{}/tcp/{}/p2p/{}",
                             config.local_ip_address,
                             config.port,
                             host->getId().toBase58())
              << "\n";
  });

  // gracefully shutdown on signal
  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
      [&io](const boost::system::error_code &, int) { io->stop(); });

  // run event loop
  io->run();
  std::cerr << "Node stopped\n";

  return 0;
}
