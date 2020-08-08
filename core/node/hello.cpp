/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/host.hpp>

#include "common/libp2p/cbor_stream.hpp"
#include "node/hello.hpp"
#include "storage/chain/chain_store.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

namespace fc::hello {
  using common::libp2p::CborStream;

  static constexpr auto kProtocolId{"/fil/hello/1.0.0"};

  Hello::Hello(std::shared_ptr<Host> host,
               std::shared_ptr<ChainStore> chain_store,
               StateCb state_cb)
      : MOVE(host), chain_store{chain_store} {
    this->host->setProtocolHandler(
        kProtocolId,
        [genesis{chain_store->genesisCid()}, MOVE(state_cb)](auto _stream) {
          auto stream{std::make_shared<CborStream>(_stream)};
          stream->template read<State>([stream, MOVE(genesis), MOVE(state_cb)](
                                           auto _state) {
            if (_state) {
              auto &state{_state.value()};
              if (auto _peer{stream->stream()->remotePeerId()}) {
                auto &peer{_peer.value()};
                if (state.genesis != genesis) {
                  spdlog::warn("Hello: peer {} has different genesis {}",
                               peer.toBase58(),
                               state.genesis);
                  return stream->stream()->reset();
                }
                stream->write(Latency{}, [stream](auto) { stream->close(); });
                state_cb(std::move(peer), std::move(state));
              } else {
                stream->stream()->reset();
              }
            }
          });
        });
  }

  void Hello::say(const PeerInfo &peer) {
    auto &ts{chain_store->heaviestTipset()};
    State hello{std::move(ts.cids),
                ts.height,
                chain_store->getHeaviestWeight(),
                chain_store->genesisCid()};
    host->newStream(peer, kProtocolId, [MOVE(hello)](auto _stream) {
      if (_stream) {
        auto stream{std::make_shared<CborStream>(_stream.value())};
        stream->write(hello, [stream](auto _n) {
          if (!_n) {
            return stream->close();
          }
          stream->template read<Latency>([stream](auto) { stream->close(); });
        });
      }
    });
  }
}  // namespace fc::hello
