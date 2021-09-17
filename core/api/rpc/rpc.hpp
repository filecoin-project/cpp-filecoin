/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>

#include <rapidjson/document.h>
#include <boost/variant.hpp>

#include "common/outcome.hpp"

namespace fc::api {
  using rapidjson::Document;

  struct Request {
    boost::optional<uint64_t> id;
    std::string method;
    Document params;
  };

  struct Response {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    struct Error {
      // Cannot initialize due to compiler bug
      // (https://bugs.llvm.org/show_bug.cgi?id=36684)
      int64_t code;
      std::string message;
    };

    boost::optional<uint64_t> id;
    boost::variant<Error, Document> result;
  };

  constexpr auto kInvalidParams = INT64_C(-32602);
  constexpr auto kInternalError = INT64_C(-32603);

  constexpr auto kRpcChVal{"xrpc.ch.val"};
  constexpr auto kRpcChClose{"xrpc.ch.close"};
}  // namespace fc::api

namespace fc::api::rpc {
  using rapidjson::Value;

  using OkCb = std::function<void(bool)>;
  using Respond =
      std::function<void(boost::variant<Response::Error, Document>)>;
  using Send = std::function<void(std::string, Document, OkCb)>;
  using MakeChan = std::function<uint64_t()>;

  using Method = std::function<void(const Value &, Respond, MakeChan, Send)>;

  struct Rpc {
    std::map<std::string, Method> ms;

    inline void setup(const std::string &name, Method &&method) {
      ms[name] = std::move(method);
    }
  };
}  // namespace fc::api::rpc
