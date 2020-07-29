/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/host.hpp>

#include "common/logger.hpp"
#include "node/builder.hpp"
#include "sync/peer_manager.hpp"
#include "sync/chain_db.hpp"

namespace fc {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger.get();
    }

    std::vector<std::string> toStrings(const std::vector<CID> &cids) {
      std::vector<std::string> v;
      v.reserve(cids.size());
      for (const auto &cid : cids) {
        v.push_back(cid.toString().value());
      }
      return v;
    }

  }  // namespace

  int main(int argc, char *argv[]) {
    using primitives::tipset::Tipset;
    using primitives::tipset::TipsetHash;

    node::Config config;

    if (!config.init(argc, argv)) {
      return 1;
    }

    auto res = fc::node::createNodeObjects(config);

    if (!res) {
      log()->error("Cannot initialize node: {}", res.error().message());
      return 2;
    }

    auto &o = res.value();

    std::map<TipsetHash, sync::TipsetCPtr> heads;

    auto res3 = o.chain_db->getHeads([&](boost::optional<TipsetHash> removed,
                                         boost::optional<TipsetHash> added) {
      if (added) {
        heads.insert({std::move(added.value()), {}});
      }
    });
    if (!res3) {
      log()->error("getHeads: {}", res3.error().message());
      return 3;
    }

    if (heads.empty()) {
      log()->error("no heads");
      return 4;
    }

    for (auto &[hash, ptr] : heads) {
      OUTCOME_EXCEPT(p, o.chain_db->getTipsetByHash(hash));
      log()->info("Head: {}, height={}", p->key.toPrettyString(), p->height());
      ptr = p;
    }

    o.io_context->post([&] {
      auto listen_res = o.host->listen(config.listen_address);
      if (!listen_res) {
        std::cerr << "Cannot listen to multiaddress "
                  << config.listen_address.getStringAddress() << ", "
                  << listen_res.error().message() << "\n";
        o.io_context->stop();
        return;
      }

      o.host->start();

      auto res = o.peer_manager->start(
          o.chain_db->genesisCID(),
          heads.begin()->second->key.cids(),
          0,
          0,
          [](const libp2p::peer::PeerId &peer,
             fc::outcome::result<fc::sync::Hello::Message> state) {
            if (!state) {
              log()->info("hello feedback failed for peer {}: {}",
                          peer.toBase58(),
                          state.error().message());
              return;
            }

            auto &s = state.value();

            log()->info(
                "hello feedback from peer:{}, cids:{}, height:{}, weight:{}",
                peer.toBase58(),
                fmt::join(toStrings(s.heaviest_tipset), ","),
                s.heaviest_tipset_height,
                s.heaviest_tipset_weight.str());
          });

      if (!res) {
        o.io_context->stop();
      }

      for (const auto &pi : config.bootstrap_list) {
        o.host->connect(pi);
      }

      // o.gossip->start();

      log()->info("Node started");
    });

    // gracefully shutdown on signal
    boost::asio::signal_set signals(*o.io_context, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code &, int) {
      o.io_context->stop();
    });

    // run event loop
    o.io_context->run();
    log()->info("Node stopped");

    return 0;
  }

}  // namespace fc

int main(int argc, char *argv[]) {
  return fc::main(argc, argv);
}
