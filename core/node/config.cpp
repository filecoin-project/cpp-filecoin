/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "config.hpp"

#include <fstream>
#include <iostream>

#include <spdlog/fmt/fmt.h>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fc::node {

  namespace {
    constexpr int kDefaultPort = 2000;
    constexpr std::string_view kDefaultConfigFileName = "default.cfg";

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

    struct ConfigRaw {
      char log_level = 'i';
      int port = 0;
      std::string car_file_name;
      std::string storage_path;
      std::string genesis_cid;
      std::vector<std::string> bootstrap_list;
    };

    bool parseCommandLine(int argc, char **argv, ConfigRaw &raw) {
      namespace po = boost::program_options;
      std::string config_file;

      po::options_description desc("Fuhon node options");
      desc.add_options()("help,h", "print usage message")(
          "config,c",
          po::value(&config_file),
          "config file name (default.cfg)")(
          "storage,s", po::value(&raw.storage_path), "storage path")(
          "port,p", po::value(&raw.port), "port to listen to")(
          "bootstrap,b",
          po::value(&raw.bootstrap_list)->composing(),
          "remote bootstrap peer uri to connect to")(
          "log,l", po::value(&raw.log_level), "log level, [e,w,i,d,t]")(
          "init",
          po::value(&raw.car_file_name),
          "initialize new storage: genesis car file name")(
          "genesis-cid,g", po::value(&raw.genesis_cid), "genesis CID");

      po::variables_map vm;
      po::store(parse_command_line(argc, argv, desc), vm);

      // if config specified explicitly, then it must exist
      bool config_file_must_exist = vm.count("config") > 0;
      if (config_file_must_exist) {
        config_file = vm["config"].as<std::string>();
      } else {
        // will be parsing default config file, if it exists
        config_file = kDefaultConfigFileName;
      }

      std::ifstream ifs(config_file.c_str());
      if (ifs.fail()) {
        if (config_file_must_exist) {
          std::cerr << "Cannot open config file " << config_file << std::endl;
          return false;
        }
      } else {
        po::store(po::parse_config_file(ifs, desc), vm);
      }

      po::notify(vm);

      if (vm.count("help") != 0) {
        std::cerr << desc << "\n";
        return false;
      }

      return true;
    }

    bool applyRawConfig(ConfigRaw &raw, fc::node::Config &config) {
      if (raw.storage_path.empty()) {
        std::cerr << "Storage path must be specified\n";
        return false;
      }

      boost::filesystem::create_directories(raw.storage_path);
      config.storage_path =
          boost::filesystem::canonical(raw.storage_path).string();

      if (raw.port != 0) {
        config.listen_address = getListenAddress(raw.port);
      }

      std::string summary = fmt::format(
          "Node is going to run with the following settings:\n"
          "\tStorage path: {}\n\tListen address: {}\n",
          config.storage_path,
          config.listen_address.getStringAddress());

      if (!raw.car_file_name.empty()) {
        config.car_file_name =
            boost::filesystem::canonical(raw.car_file_name).string();
        summary += fmt::format("\tNew storage to be initialized from {}\n",
                               config.car_file_name);
      }

      if (!raw.genesis_cid.empty()) {
        auto res = CID::fromString(raw.genesis_cid);
        if (!res) {
          std::cerr << "Cannot decode CID from string " << raw.genesis_cid
                    << "\n";
          return false;
        }
        config.genesis_cid = std::move(res.value());
        summary +=
            fmt::format("\tGenesis CID must be equal to {}\n", raw.genesis_cid);
      }

      if (!raw.bootstrap_list.empty()) {
        summary += fmt::format("\tBootstrap peers:\n");

        for (const auto &b : raw.bootstrap_list) {
          auto res = str2peerInfo(b);
          if (!res) {
            std::cerr << "Cannot resolve remote peer address from " << b
                      << "\n";
            return false;
          }
          config.bootstrap_list.push_back(res.value());
          summary += fmt::format("\t\t{}\n", b);
        }
      }

      config.log_level = getLogLevel(raw.log_level);
      spdlog::set_level(config.log_level);
      summary += fmt::format(
          "\tLog level: {}\n\tLocal IP: {}", raw.log_level, config.local_ip);

      spdlog::info(summary);

      return true;
    }

  }  // namespace

  Config::Config()
      : log_level(spdlog::level::info),
        listen_address(getListenAddress(kDefaultPort)),
        local_ip(getLocalIP()) {}

  bool Config::init(int argc, char *argv[]) {
    try {
      ConfigRaw raw;
      return parseCommandLine(argc, argv, raw) && applyRawConfig(raw, *this);
    } catch (const std::exception &e) {
      std::cerr << "Cannot parse options: " << e.what() << "\n";
    }
    return false;
  }

}  // namespace fc::node
