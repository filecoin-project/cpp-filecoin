/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/host.hpp>

#include "common/logger.hpp"
#include "node/builder.hpp"
#include "sync/chain_db.hpp"
#include "sync/peer_manager.hpp"
#include "sync/sync_job.hpp"
#include "sync/tipset_loader.hpp"
#include "vm/interpreter/interpreter.hpp"

#include "sync/index_db_backend.hpp"
#include "sync/pubsub_gate.hpp"

namespace fc {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("node");
      return logger.get();
    }

    [[maybe_unused]] std::vector<std::string> toStrings(const std::vector<CID> &cids) {
      std::vector<std::string> v;
      v.reserve(cids.size());
      for (const auto &cid : cids) {
        v.push_back(cid.toString().value());
      }
      return v;
    }

  }  // namespace

  struct PubsubCtx {
    node::NodeObjects &o;
    std::string network_name;
    std::shared_ptr<sync::PubSubGate> pubsub;
    sync::PubSubGate::connection_t blocks_subscr;
    sync::PubSubGate::connection_t msg_subscr;

    PubsubCtx(node::NodeObjects &objects, const std::string &nname)
        : o(objects),
          network_name(std::move(nname)),
          pubsub(std::make_shared<sync::PubSubGate>(o.utc_clock, o.gossip)) {}

    void start() {
      pubsub->start(network_name, "X");
      blocks_subscr = pubsub->subscribeToBlocks([](const sync::PeerId &from,
                                                   const CID &block_cid,
                                                   const sync::BlockMsg &msg) {
        log()->info("New block from {}, cid={}, height={}",
                    from.toBase58(),
                    block_cid.toString().value(),
                    msg.header.height);
      });

      msg_subscr = pubsub->subscribeToMessages(
          [](const sync::PeerId &from,
             const CID &cid,
             const common::Buffer &raw,
             const sync::UnsignedMessage &msg,
             boost::optional<std::reference_wrapper<const sync::Signature>>
                 signature) {
            log()->info("New msg from {}, cid={}, secp={}",
                        from.toBase58(),
                        cid.toString().value(),
                        signature.has_value());
          });
    }
  };

  struct SyncerCtx {
    node::NodeObjects &o;
    sync::Syncer syncer;
    bool started = false;
    libp2p::protocol::scheduler::Handle sch_handle;

    SyncerCtx(node::NodeObjects &objects)
        : o(objects),
          syncer(o.scheduler,
                 o.tipset_loader,
                 o.chain_db,
                 [](sync::SyncStatus st) {
                   log()->info("Sync status {}, total={}", st.code, st.total);
                   // o.io_context->stop();
                 }) {}

    void start() {
      if (!started) {
        if (!o.chain_db->start([](std::vector<sync::TipsetHash> removed,
                                  std::vector<sync::TipsetHash> added) {
              log()->info("Head change callback called");
            })) {
          o.io_context->stop();
          return;
        }
        started = true;
        sch_handle = o.scheduler->schedule(2000, [this] { syncer.start(); });
      }
    }
  };

  int reindex(node::NodeObjects &o, const node::Config &config) {
    OUTCOME_EXCEPT(
        old_idx,
        sync::IndexDbBackend::create(config.storage_path + "/index.db.old"));
    OUTCOME_EXCEPT(
        new_idx,
        sync::IndexDbBackend::create(config.storage_path + "/index.db.new"));

    auto branches = old_idx->initDb().value();
    std::ignore = new_idx->initDb().value();

    auto cb = [&](sync::IndexDbBackend::TipsetIdx idx) {
      if (idx.height % 1000 == 0) log()->info("reindex height {}", idx.height);
      OUTCOME_EXCEPT(buffer, o.kvstorage->get(common::Buffer(idx.hash)));
      OUTCOME_EXCEPT(cids, codec::cbor::decode<std::vector<CID>>(buffer));
      sync::TipsetInfo info{
          sync::TipsetKey::create(std::move(cids), std::move(idx.hash)),
          idx.branch,
          idx.height,
          std::move(idx.parent_hash)};
      OUTCOME_EXCEPT(new_idx->store(info, boost::none));
    };

    for (const auto &[branch, _] : branches) {
      log()->info("reindex branch {}", branch);
      OUTCOME_EXCEPT(old_idx->walk(branch, 0, -1, cb));
    }

    return 0;
  }

  int verify(const std::string &db) {
    OUTCOME_EXCEPT(new_idx, sync::IndexDbBackend::create(db));

    auto branches = new_idx->initDb().value();

    auto verify_cb = [&](sync::IndexDbBackend::TipsetIdx idx) {
      if (idx.height % 1000 == 0) log()->info("verify height {}", idx.height);
      OUTCOME_EXCEPT(info, sync::IndexDbBackend::decode(idx));
      if (info->key.hash()
          != primitives::tipset::tipsetHash(info->key.cids()).value()) {
        log()->error("verify error at height {}", idx.height);
      }
    };

    for (const auto &[branch, _] : branches) {
      log()->info("verify branch {}", branch);
      OUTCOME_EXCEPT(new_idx->walk(branch, 0, -1, verify_cb));
    }

    return 0;
  }

  int main(int argc, char *argv[]) {
    using primitives::tipset::Tipset;
    using primitives::tipset::TipsetHash;

    node::Config config;

    if (!config.init(argc, argv)) {
      return 1;
    }

    // suppress verbose logging from secio
    common::createLogger("SECCONN")->set_level(spdlog::level::info);
    common::createLogger("SECIO")->set_level(spdlog::level::info);

    auto res = node::createNodeObjects(config);

    if (!res) {
      log()->error("Cannot initialize node: {}", res.error().message());
      return 2;
    }

    auto &o = res.value();

    if (std::getenv("REINDEX") != nullptr) {
      int r = reindex(o, config);
      if (r == 0) {
        r = verify(config.storage_path + "/index.db.new");
      }
      return r;
    }

    if (std::getenv("VERIFY_INDEX") != nullptr) {
      return verify(config.storage_path + "/index.db");
    }

    std::map<TipsetHash, sync::TipsetCPtr> heads;

    auto res3 = o.chain_db->getHeads(
        [&](std::vector<TipsetHash> removed, std::vector<TipsetHash> added) {
          for (auto &hash : added) {
            heads.insert({std::move(hash), {}});
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

    OUTCOME_EXCEPT(o.chain_db->setCurrentHead(heads.begin()->first));

    if (std::getenv("VM_INTERPRET") != nullptr) {
      size_t tipsets_interpreted = 0;
      size_t mismatches = 0;
      bool interpret_error = false;

      vm::interpreter::Result result;

      OUTCOME_EXCEPT(
          o.chain_db->walkForward(0, -1, [&](sync::TipsetCPtr tipset) {
            if (!interpret_error) {
              if (tipset->height() % 1000 == 0) {
                log()->info("interpreting at {}", tipset->height());
              }
              auto r = o.vm_interpreter->interpret(o.ipld, tipset);
              if (!r) {
                log()->error("Interpret error at height {} : {}",
                             tipset->height(),
                             r.error().message());
                interpret_error = true;
              } else {
                if (tipset->height() > 0) {
                  if (result.message_receipts
                          != tipset->getParentMessageReceipts()
                      || result.state_root != tipset->getParentStateRoot()) {
                    ++mismatches;
                  }
                }
                result = std::move(r.value());
                ++tipsets_interpreted;
              }
            }
          }));

      log()->info("interpreted {} tipsets, found {} mismatches",
                  tipsets_interpreted,
                  mismatches);

      return 0;
    }

    if (std::getenv("WALK_FWD") != nullptr) {
      size_t tipsets_visited = 0;
      OUTCOME_EXCEPT(
          o.chain_db->walkForward(0, 10000, [&](sync::TipsetCPtr tipset) {
            if (tipset->height() % 1000 == 0) {
              log()->info("walking fwd at {}", tipset->height());
            }
            ++tipsets_visited;
          }));
      log()->info("visited {} tipsets", tipsets_visited);
      return 0;
    }

    if (std::getenv("WALK_BWD") != nullptr) {
      size_t tipsets_visited = 0;
      TipsetHash from;
      if (heads.begin()->second->height() <= 10000) {
        from = heads.begin()->first;
      } else {
        OUTCOME_EXCEPT(ts, o.chain_db->getTipsetByHeight(10000));
        from = std::move(ts->key.hash());
      }
      OUTCOME_EXCEPT(
          o.chain_db->walkBackward(from, 0, [&](sync::TipsetCPtr tipset) {
            if (tipset->height() % 1000 == 0) {
              log()->info("walking bwd at {}", tipset->height());
            }
            ++tipsets_visited;
          }));
      log()->info("visited {} tipsets", tipsets_visited);
      return 0;
    }

    SyncerCtx ctx(o);

    PubsubCtx pubsub_ctx(o, config.network_name);

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
          [&](const libp2p::peer::PeerId &peer,
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

            auto tk = primitives::tipset::TipsetKey::create(s.heaviest_tipset);
            if (tk) {
              ctx.syncer.newTarget(peer,
                                   std::move(tk.value()),
                                   s.heaviest_tipset_weight,
                                   s.heaviest_tipset_height);
              ctx.start();
            }

   //         o.gossip->addBootstrapPeer(peer, boost::none);
          });

      if (!res) {
        o.io_context->stop();
      }

      for (const auto &pi : config.bootstrap_list) {
        o.host->connect(pi);
       // o.gossip->addBootstrapPeer(pi.id, pi.addresses[0]);
      }

      o.gossip->start();

      pubsub_ctx.start();

      log()->info("Node started");
    });

    // gracefully shutdown on signal
    boost::asio::signal_set signals(*o.io_context, SIGINT, SIGTERM);
    signals.async_wait(
        [&](const boost::system::error_code &, int) { o.io_context->stop(); });

    // run event loop
    o.io_context->run();
    log()->info("Node stopped");

    return 0;
  }

}  // namespace fc

int main(int argc, char *argv[]) {
  return fc::main(argc, argv);
}
