/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drand/impl/http.hpp"

#include <rapidjson/document.h>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

#define EC_CB()    \
  if (ec) {        \
    return cb(ec); \
  }

OUTCOME_CPP_DEFINE_CATEGORY(fc::drand::http, Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::kJson:
      return "Drand bad json";
  }
}

namespace fc::drand::http {
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace net = boost::asio;
  using tcp = net::ip::tcp;

  using rapidjson::Value;

  template <typename Parse, typename Next>
  inline auto withJson(Parse &&parse, Next &&next) {
    return [MOVE(parse), MOVE(next)](auto &&_str) {
      if (!_str) {
        return next(_str.error());
      }
      auto &str{_str.value()};
      rapidjson::Document j;
      j.Parse(str.data(), str.size());
      if (j.HasParseError()) {
        return next(Error::kJson);
      }
      next(parse(std::move(j)));
    };
  }

  const Value *get(const Value &j, const char *key) {
    if (j.IsObject()) {
      auto it{j.FindMember(key)};
      if (it != j.MemberEnd()) {
        return &it->value;
      }
    }
    return {};
  }

  outcome::result<uint64_t> getUint(const Value *j) {
    if (!j || !j->IsUint64()) {
      return Error::kJson;
    }
    return j->GetUint64();
  }

  outcome::result<int64_t> getInt(const Value *j) {
    if (!j || !j->IsInt64()) {
      return Error::kJson;
    }
    return j->GetInt64();
  }

  template <typename T>
  outcome::result<void> unhex(T &t, const Value *j) {
    if (!j || !j->IsString()) {
      return Error::kJson;
    }
    OUTCOME_TRYA(t, T::fromHex({j->GetString(), j->GetStringLength()}));
    return outcome::success();
  }

  struct ClientSession {
    ClientSession(io_context &io)
        : resolver{io}, stream{net::make_strand(io)} {}
    ~ClientSession() {
      boost::system::error_code ec;
      stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }

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
               std::string host,
               std::function<void(outcome::result<ChainInfo>)> cb) {
    ClientSession::get(io,
                       host,
                       "/info",
                       withJson(
                           [](auto &&j) -> outcome::result<ChainInfo> {
                             ChainInfo r;
                             OUTCOME_TRY(unhex(r.key, get(j, "public_key")));
                             OUTCOME_TRY(period, getUint(get(j, "period")));
                             r.period = seconds{period};
                             OUTCOME_TRY(genesis,
                                         getInt(get(j, "genesis_time")));
                             r.genesis = seconds{genesis};
                             return r;
                           },
                           std::move(cb)));
  }

  void getEntry(io_context &io,
                std::string host,
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
            [](auto &&j) -> outcome::result<PublicRandResponse> {
              PublicRandResponse r;
              OUTCOME_TRYA(r.round, getUint(get(j, "round")));
              OUTCOME_TRY(unhex(r.signature, get(j, "signature")));
              OUTCOME_TRY(unhex(r.prev, get(j, "previous_signature")));
              if (r.prev.size()
                  != (r.round == 1 ? common::Hash256::size()
                                   : BlsSignature::size())) {
                return common::BlobError::kIncorrectLength;
              }
              return r;
            },
            std::move(cb)));
  }
}  // namespace fc::drand::http
