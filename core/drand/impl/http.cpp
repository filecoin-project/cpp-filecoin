/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drand/impl/http.hpp"

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "codec/json/json.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

#define EC_CB()    \
  if (ec) {        \
    return cb(ec); \
  }

namespace fc::drand::http {
  using codec::json::jGet;
  using codec::json::jInt;
  using codec::json::jUint;
  using codec::json::jUnhex;
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace net = boost::asio;
  using tcp = net::ip::tcp;

  namespace Json = codec::json;

  template <typename Parse, typename Next>
  inline auto withJson(Parse &&parse, Next &&cb) {
    return [parse{std::forward<Parse>(parse)},
            cb{std::forward<Next>(cb)}](auto &&_str) {
      OUTCOME_CB(auto str, _str);
      OUTCOME_CB(auto jdoc, Json::parse(str));
      cb(outcomeCatch([&] { return parse(&jdoc); }));
    };
  }

  struct ClientSession {
    explicit ClientSession(io_context &io)
        : resolver{io}, stream{net::make_strand(io)} {}
    ClientSession(const ClientSession &) = delete;
    ClientSession(ClientSession &&) = delete;
    ~ClientSession() {
      boost::system::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    ClientSession &operator=(const ClientSession &) = delete;
    ClientSession &operator=(ClientSession &&) = delete;

    // config: port, timeout
    static void get(io_context &io,
                    const std::string &host,
                    const std::string &target,
                    std::function<void(outcome::result<std::string>)> cb) {
      auto s{std::make_shared<ClientSession>(io)};
      s->req.method(http::verb::get);
      s->req.target(target);
      s->req.set(http::field::host, host);
      s->stream.expires_after(std::chrono::seconds(5));
      s->resolver.async_resolve(
          host, "80", [s, MOVE(cb)](auto &&ec, auto &&iterator) {
            EC_CB();
            s->stream.async_connect(
                iterator, [s, MOVE(cb)](auto &&ec, auto &&) {
                  EC_CB();
                  http::async_write(
                      s->stream, s->req, [s, MOVE(cb)](auto &&ec, auto &&) {
                        EC_CB();
                        http::async_read(s->stream,
                                         s->buffer,
                                         s->res,
                                         [s, MOVE(cb)](auto &&ec, auto &&) {
                                           EC_CB();
                                           cb(std::move(s->res.body()));
                                         });
                      });
                });
          });
    }

    tcp::resolver resolver;
    beast::tcp_stream stream;
    http::request<http::empty_body> req;
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
  };

  void getInfo(io_context &io,
               const std::string &host,
               std::function<void(outcome::result<ChainInfo>)> cb) {
    ClientSession::get(
        io,
        host,
        "/info",
        withJson(
            [](auto &&j) {
              return ChainInfo{
                  BlsPublicKey::fromSpan(*jUnhex(*jGet(j, "public_key")))
                      .value(),
                  seconds{*jInt(*jGet(j, "genesis_time"))},
                  seconds{*jUint(*jGet(j, "period"))},
              };
            },
            std::move(cb)));
  }

  void getEntry(io_context &io,
                const std::string &host,
                uint64_t round,
                std::function<void(outcome::result<PublicRandResponse>)> cb) {
    std::string url{"/public/"};
    if (round == 0) {
      url += "latest";
    } else {
      url += std::to_string(round);
    }
    ClientSession::get(
        io,
        host,
        url,
        withJson(
            [](auto &&j) {
              PublicRandResponse r{
                  *jUint(*jGet(j, "round")),
                  BlsSignature::fromSpan(*jUnhex(*jGet(j, "signature")))
                      .value(),
                  *jUnhex(*jGet(j, "previous_signature")),
              };
              if (r.prev.size()
                  != (r.round == 1 ? common::Hash256::size()
                                   : BlsSignature::size())) {
                outcome::raise(common::BlobError::kIncorrectLength);
              }
              return r;
            },
            std::move(cb)));
  }
}  // namespace fc::drand::http
