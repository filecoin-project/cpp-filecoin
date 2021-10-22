/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/libp2p/stream_open_queue.hpp"

namespace libp2p::connection {
  StreamOpenQueue::Active::Active(std::shared_ptr<Stream> stream,
                                  std::weak_ptr<StreamOpenQueue> weak,
                                  List::iterator it)
      : StreamProxy{std::move(stream)},
        weak{std::move(weak)},
        it{std::move(it)} {}

  StreamOpenQueue::Active::~Active() {
    stream->close([stream{stream}](auto) { stream->reset(); });
    if (auto queue{weak.lock()}) {
      queue->active.erase(it);
      queue->check();
    }
  }

  StreamOpenQueue::StreamOpenQueue(std::shared_ptr<Host> host,
                                   size_t max_active)
      : host{std::move(host)}, max_active{max_active} {
    assert(max_active >= 1);
  }

  void StreamOpenQueue::open(Pending item) {
    queue.emplace(std::move(item));
    check();
  }

  void StreamOpenQueue::check() {
    while (!queue.empty() && active.size() < max_active) {
      auto item{std::move(queue.front())};
      queue.pop();
      auto it{active.insert(active.begin(), Active::List::value_type{})};
      host->newStream(item.peer,
                      item.protocol,
                      [weak{weak_from_this()},
                       cb{std::move(item.cb)},
                       it{std::move(it)}](Host::StreamResult _stream) {
                        if (auto self{weak.lock()}) {
                          if (!_stream) {
                            self->active.erase(it);
                            self->check();
                            return cb(_stream);
                          }
                          auto stream{std::make_shared<Active>(
                              std::move(_stream.value()), weak, it)};
                          *it = stream;
                          cb(std::move(stream));
                        }
                      });
    }
  }
}  // namespace libp2p::connection
