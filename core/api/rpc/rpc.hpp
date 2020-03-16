/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_API_RPC_RPC_HPP
#define CPP_FILECOIN_CORE_API_RPC_RPC_HPP

#include <map>

#include <rapidjson/document.h>

#include "common/outcome.hpp"

namespace fc::api::rpc {
  using rapidjson::Document;
  using rapidjson::Value;

  using Method = std::function<outcome::result<Document>(const Value &)>;

  struct Rpc {
    std::map<std::string, Method> ms;

    inline void setup(const std::string &name, Method &&method) {
      ms.emplace(name, std::move(method));
    }
  };
}  // namespace fc::api::rpc

#endif  // CPP_FILECOIN_CORE_API_RPC_RPC_HPP
