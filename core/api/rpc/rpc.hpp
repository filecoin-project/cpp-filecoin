/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_RPC_HPP
#define CPP_FILECOIN_CORE_API_RPC_RPC_HPP

#include <map>

#include <rapidjson/document.h>
#include <boost/signals2.hpp>

#include "common/outcome.hpp"

namespace fc::api {
  using rapidjson::Document;

  struct Request {
    uint64_t id;
    std::string method;
    Document params;
  };

  struct Response {
    struct Error {
      int64_t code;
      std::string message;
    };

    boost::optional<uint64_t> id;
    boost::variant<Error, Document> result;
  };
}  // namespace fc::api

namespace fc::api::rpc {
  using rapidjson::Value;

  using Send = std::function<void(Request, std::function<void(bool)>)>;
  using Maker = std::function<void(Send)>;
  using MakeChan = std::function<uint64_t(Maker)>;

  using Method =
      std::function<outcome::result<Document>(const Value &, MakeChan)>;

  struct Rpc {
    std::map<std::string, Method> ms;
    std::map<uint64_t, boost::signals2::connection> channels;

    inline void setup(const std::string &name, Method &&method) {
      ms.emplace(name, std::move(method));
    }
  };
}  // namespace fc::api::rpc

#endif  // CPP_FILECOIN_CORE_API_RPC_RPC_HPP
