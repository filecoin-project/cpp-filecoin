/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fstream>
#include <iostream>

#include <spdlog/fmt/fmt.h>

#include <boost/di/extension/scopes/shared.hpp>
#include <boost/program_options.hpp>

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/identify.hpp>

#include <libp2p/injector/gossip_injector.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/identify/identify_delta.hpp>
#include <libp2p/protocol/identify/identify_push.hpp>

#include "blockchain/impl/weight_calculator_impl.hpp"
#include "clock/impl/chain_epoch_clock_impl.hpp"
#include "clock/impl/utc_clock_impl.hpp"
#include "common/logger.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/car/car.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "sync/block_loader.hpp"
#include "sync/blocksync_client.hpp"
#include "sync/hello.hpp"
#include "sync/peer_manager.hpp"
#include "sync/tipset_loader.hpp"
#include "sync/index_db_backend.hpp"
#include "storage/leveldb/leveldb.hpp"

std::ostream &operator<<(std::ostream &os, const fc::CID &cid) {
  os << cid.toString().value();
  return os;
}

namespace {
  auto log() {
    static fc::common::Logger logger = fc::common::createLogger("integration");
    return logger.get();
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

  bool tryLoadTipset(fc::sync::TipsetLoader &loader,
                     std::vector<fc::CID> cids,
                     boost::optional<libp2p::peer::PeerId> peer) {
    std::error_code e;
    auto res = fc::sync::TipsetKey::create(std::move(cids));
    if (res) {
      log()->info("loading tipset...");
      auto load_res = loader.loadTipsetAsync(res.value(), peer, 25);
      if (!load_res) {
        e = load_res.error();
      }
    } else {
      e = res.error();
    }

    if (e) {
      log()->error("cannot start loading tipset, {}", e.message());
    }

    return (!e);
  }

  void helloFeedback(std::shared_ptr<fc::sync::TipsetLoader> loader,
                     std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
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

    scheduler->schedule( 3000, [loader, s=std::move(s), peer] {
      tryLoadTipset(*loader, std::move(s.heaviest_tipset), peer);
    }).detach();
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

  fc::outcome::result<void> loadCar(const std::string &file_name,
                                    fc::storage::ipfs::IpfsDatastore &storage,
                                    const fc::CID &expected_genesis) {
    // if genesis in storage then all is ok

    std::ifstream file{file_name, std::ios::binary | std::ios::ate};
    if (!file.good()) {
      log()->error("cannot open file {}", file_name);
      return fc::sync::Error::SYNC_NOT_INITIALIZED;
    }

    static const size_t kMaxSize = 1024 * 1024;
    auto size = static_cast<size_t>(file.tellg());
    if (size > kMaxSize) {
      log()->error(
          "file size above expected, file:{}, size:{}", file_name, size);
      return fc::sync::Error::SYNC_NOT_INITIALIZED;
    }

    std::string buffer;
    buffer.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), buffer.size());

    auto result = fc::storage::car::loadCar(storage, toSpanU8(buffer));
    if (!result) {
      log()->error(
          "cannot load car file {}: {}", file_name, result.error().message());
      return result.error();
    }

    auto contains = storage.contains(expected_genesis);
    if (!contains) {
      log()->error("cannot load expected genesis cid:{} from car file:{}, {}",
                   expected_genesis.toString().value(),
                   file_name,
                   contains.error().message());
      return result.error();
    }

    if (!contains.value()) {
      log()->error(
          "cannot load expected genesis cid:{} from car file:{} which "
          "contains: {}",
          expected_genesis.toString().value(),
          file_name,
          fmt::join(toStrings(result.value()), " "));
      return fc::sync::Error::SYNC_NOT_INITIALIZED;
    }

    log()->debug("car file {} contains {}",
                 file_name,
                 fmt::join(toStrings(result.value()), " "));
    return fc::outcome::success();
  }

}  // namespace

int main(int argc, char *argv[]) {
  fc::node::Config config;

  if (!config.init("", argc, argv)) {
    return 1;
  }

  fc::common::createLogger("sync")->set_level(spdlog::level::debug);

  //try {
    auto injector = libp2p::injector::makeGossipInjector<
        boost::di::extension::shared_config>(
        boost::di::bind<fc::clock::UTCClock>.template to<fc::clock::UTCClockImpl>(),
 //       boost::di::bind<libp2p::security::SecurityAdaptor *[]>().template to<libp2p::security::Plaintext>()[boost::di::override],
        libp2p::injector::useGossipConfig(config.gossip_config));

    auto io = injector.create<std::shared_ptr<boost::asio::io_context>>();
    auto scheduler =
        injector.create<std::shared_ptr<libp2p::protocol::Scheduler>>();
    auto host = injector.create<std::shared_ptr<libp2p::Host>>();
    auto identify_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::Identify>>();
    auto identify_push_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::IdentifyPush>>();
    auto identify_delta_protocol =
        injector.create<std::shared_ptr<libp2p::protocol::IdentifyDelta>>();
    auto utc_clock = injector.create<std::shared_ptr<fc::clock::UTCClock>>();
    auto ipld = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();

    //OUTCOME_EXCEPT(leveldb, fc::storage::LevelDB::create("ldb");


    auto weight_calculator =
        std::make_shared<fc::blockchain::weight::WeightCalculatorImpl>(ipld);
    auto peer_manager =
        std::make_shared<fc::sync::PeerManager>(host,
                                                utc_clock,
                                                identify_protocol,
                                                identify_push_protocol,
                                                identify_delta_protocol);

    auto blocksync =
        std::make_shared<fc::sync::blocksync::BlocksyncClient>(host, ipld);

    auto block_loader =
        std::make_shared<fc::sync::BlockLoader>(ipld, scheduler, blocksync);

    auto tipset_loader =
        std::make_shared<fc::sync::TipsetLoader>(scheduler, block_loader);



    //OUTCOME_TRY(index_db_backend, fc::sync::IndexDbBackend::create("x.db"));
    //OUTCOME_TRY(branches, index_db_backend->initDb());
    //auto IndexDb = std::make_shared<fc::sync::IndexDb>("x.db");


//    OUTCOME_EXCEPT(
//        loadCar(config.storage_car_file_name, *ipld, config.genesis_cid));

    //auto genesis_tipset = fc::sync::Tipset::load(*ipld, {config.genesis_cid});

    //if (!genesis_tipset) {
    //  throw std::system_error(genesis_tipset.error());
    //}

    //auto genesis_weight =
    //    weight_calculator->calculateWeight(genesis_tipset.value());

    //if (!genesis_weight) {
    //  throw std::system_error(genesis_weight.error());
    //}

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

      host->start();

      tipset_loader->init([=](fc::sync::TipsetHash hash,
                              fc::outcome::result<fc::sync::Tipset> tipset) {
        if (!tipset) {
          log()->error("tipset load error, {}", tipset.error().message());
          io->stop();
        } else {
          auto h = tipset.value().height();
          log()->info("tipset loaded at height {}", h);
          if (h > 0) {
            if (!tryLoadTipset(*tipset_loader,
                               tipset.value().blks[0].parents,
                               boost::none)) {
              io->stop();
            }
          } else {
            io->stop();
          }
        }
      });

      peer_manager->start(
          config.genesis_cid,
          { config.genesis_cid },
          1213,
          0,
          //genesis_tipset.value(),
          //genesis_weight.value(),
          [tipset_loader, scheduler](const libp2p::peer::PeerId &peer,
                          fc::outcome::result<fc::sync::Hello::Message> state) {
            helloFeedback(tipset_loader, scheduler, peer, std::move(state));
          });

      for (const auto &pi : config.bootstrap_list) {
        ///ip4/192.168.0.103/tcp/37613/p2p/12D3KooWDvaty58tskHsHKAR9qhLRxyKZfez8WmfgmASqibQeRCd
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
  //} catch (const std::exception &e) {
  //  std::cerr << e.what();
  //  return 2;
  //}
  return 0;
}
