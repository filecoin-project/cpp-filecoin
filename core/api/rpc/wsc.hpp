/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <future>
#include <libp2p/multi/multiaddress.hpp>
#include <mutex>
#include <queue>
#include <thread>

#include "api/api.hpp"
#include "api/rpc/rpc.hpp"

namespace fc::api::rpc {
  using boost::asio::io_context;
  using libp2p::multi::Multiaddress;

  struct Client {
    using ResultCb = std::function<void(outcome::result<Document>)>;
    using ChanCb = std::function<bool(boost::optional<Document>)>;

    Client(io_context &io2);
    ~Client();
    outcome::result<void> connect(const Multiaddress &address,
                                  const std::string &token);
    void call(Request &&req, ResultCb &&cb);
    void _chan(uint64_t id, ChanCb &&cb);
    void _error(const std::error_code &error);
    void _flush();
    void _read();
    void _onread(const Document &j);
    void setup(Api &api);

    std::thread thread;
    io_context io;
    io_context &io2;
    boost::asio::executor_work_guard<io_context::executor_type> work_guard;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> socket;
    boost::beast::flat_buffer buffer;
    std::mutex mutex;
    uint64_t next_req{};
    std::map<uint64_t, ResultCb> result_queue;
    std::map<uint64_t, ChanCb> chans;
    std::queue<std::pair<uint64_t, Buffer>> write_queue;
    bool writing{false};
  };
}  // namespace fc::api::rpc
