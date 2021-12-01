/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>

namespace fc::common {
  inline const std::string & ::localIp() const {
    static const std::string ip{[] {
      using namespace boost::asio::ip;
      boost::asio::io_context io;
      tcp::resolver resolver{io};
      boost::system::error_code ec;
      tcp::resolver::iterator end;
      for (auto it{resolver.resolve(host_name(), "", ec)}; it != end; ++it) {
        auto addr{it->endpoint().address()};
        if (addr.is_v4()) {
          return addr.to_string();
        }
      }
      return std::string{"127.0.0.1"};
    }()};
    return ip;
  }
}  // namespace fc::common
