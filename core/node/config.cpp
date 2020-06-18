/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.hpp"

#include <iostream>

#include <spdlog/fmt/fmt.h>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

namespace fc::node {

  namespace {
    const std::string &getLocalIP() {
      static const std::string a = [] {
        boost::asio::io_context io;
        boost::asio::ip::tcp::resolver resolver(io);
        boost::asio::ip::tcp::resolver::query query(
            boost::asio::ip::host_name(), "");
        boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
        boost::asio::ip::tcp::resolver::iterator end;
        std::string addr("127.0.0.1");
        while (it != end) {
          auto ep = it->endpoint();
          if (ep.address().is_v4()) {
            addr = ep.address().to_string();
            break;
          }
          ++it;
        }
        return addr;
      }();

      return a;
    }

    libp2p::multi::Multiaddress getListenAddress(int port) {
      return libp2p::multi::Multiaddress::create(
                 fmt::format("/ip4/0.0.0.0/tcp/{}", port))
          .value();
    }

    const CID &getDefaultGenesisCID() {
      static const CID cid =
          CID::fromString(
              "bafy2bzacecbcmikekv2hvyqprfj6dyvbklvdeuht3mr736owhovfx75hops7m")
              .value();
      return cid;
    }

    boost::optional<libp2p::peer::PeerInfo> str2peerInfo(
        const std::string &str) {
      auto ma_res = libp2p::multi::Multiaddress::create(str);
      if (!ma_res) {
        return boost::none;
      }
      auto ma = std::move(ma_res.value());

      auto peer_id_str = ma.getPeerId();
      if (!peer_id_str) {
        return boost::none;
      }

      auto peer_id_res = libp2p::peer::PeerId::fromBase58(*peer_id_str);
      if (!peer_id_res) {
        return boost::none;
      }

      return libp2p::peer::PeerInfo{peer_id_res.value(), {ma}};
    }

    spdlog::level::level_enum getLogLevel(char level) {
      spdlog::level::level_enum spdlog_level = spdlog::level::info;
      switch (level) {
        case 'e':
          spdlog_level = spdlog::level::err;
          break;
        case 'w':
          spdlog_level = spdlog::level::warn;
          break;
        case 'd':
          spdlog_level = spdlog::level::debug;
          break;
        case 't':
          spdlog_level = spdlog::level::trace;
          break;
        default:
          break;
      }
      return spdlog_level;
    }

    constexpr int kDefaultPort = 2000;
    constexpr std::string_view kDefaultNetworkName = "fuhon_test";
    constexpr std::string_view kDefaultCarFileName = "genesis.car";

    bool parseCommandLine(int argc, char **argv, fc::node::Config &config) {
      namespace po = boost::program_options;
      try {
        std::string remote;
        int port = 0;
        char log_level = 'i';

        po::options_description desc("Fuhon node options");
        desc.add_options()("help,h", "print usage message")(
            "port,p", po::value(&port), "port to listen to")(
            "remote,r", po::value(&remote), "remote peer uri to connect to")(
            "log,l", po::value(&log_level), "log level, [e,w,i,d,t]");

        po::variables_map vm;
        po::store(parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help") != 0) {
          std::cerr << desc << "\n";
          return false;
        }

        if (port != 0) {
          config.port = port;
          config.listen_address = getListenAddress(port);
        }

        if (!remote.empty()) {
          auto res = str2peerInfo(remote);
          if (!res) {
            std::cerr << "Cannot resolve remote peer address from " << remote
                      << "\n";
            return false;
          }
          config.bootstrap_list.push_back(res.value());
        }

        config.log_level = getLogLevel(log_level);
        spdlog::set_level(config.log_level);

        return true;

      } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
      }
      return false;
    }

  }  // namespace

  Config::Config()
      : log_level(spdlog::level::info),
        local_ip_address(getLocalIP()),
        port(kDefaultPort),
        network_name(kDefaultNetworkName),
        storage_car_file_name(kDefaultCarFileName),
        genesis_cid(getDefaultGenesisCID()),
        network_secio(false),
        listen_address(getListenAddress(kDefaultPort)) {}

  bool Config::init(const std::string & /*config_file*/,
                    int argc,
                    char *argv[]) {
    return parseCommandLine(argc, argv, *this);
  }

}  // namespace fc::node
