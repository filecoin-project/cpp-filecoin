/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_info.hpp>
#include <sstream>

using libp2p::peer::PeerInfo;

/**
 * Represent peer info as human readable string
 * @param peer_info to prettify
 * @return string
 */
inline std::string peerInfoToPrettyString(const PeerInfo &peer_info) {
  std::stringstream ss;
  for (const auto &address : peer_info.addresses) {
    ss << std::string(address.getStringAddress()) << " ";
  }
  return ss.str();
}

namespace fc {
  using libp2p::multi::Multiaddress;

  inline bool isZeroIp(const Multiaddress &addr) {
    if (auto _ip{addr.getFirstValueForProtocol(
            libp2p::multi::Protocol::Code::IP4)}) {
      return _ip.value() == "0.0.0.0";
    }
    return false;
  }

  inline boost::optional<std::string> nonZeroPort(const Multiaddress &addr) {
    if (auto _port{addr.getFirstValueForProtocol(
            libp2p::multi::Protocol::Code::TCP)}) {
      auto &port{_port.value()};
      if (port != "0") {
        return port;
      }
    }
    return boost::none;
  }

  inline boost::optional<Multiaddress> nonZeroAddr(const Multiaddress &addr,
                                                   const std::string *ip = {}) {
    if (auto port{nonZeroPort(addr)}) {
      if (isZeroIp(addr)) {
        if (ip) {
          return Multiaddress::create(fmt::format("/ip4/{}/tcp/{}", *ip, *port))
              .value();
        }
      } else {
        return addr;
      }
    }
    return boost::none;
  }

  inline boost::optional<Multiaddress> nonZeroAddr(
      const std::vector<Multiaddress> &addrs, const std::string *ip = {}) {
    for (auto &addr : addrs) {
      if (auto addr2{nonZeroAddr(addr, ip)}) {
        return addr2;
      }
    }
    return boost::none;
  }

  inline auto nonZeroAddrs(const std::vector<Multiaddress> &addrs,
                           const std::string *ip = {}) {
    std::vector<Multiaddress> addrs2;
    for (auto &addr : addrs) {
      if (auto addr2{nonZeroAddr(addr, ip)}) {
        addrs2.push_back(std::move(*addr2));
      }
    }
    return addrs2;
  }
}  // namespace fc
