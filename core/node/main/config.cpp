/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "node/main/config.hpp"

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include "config/profile_config.hpp"
#include "primitives/address/config.hpp"

namespace fc::common {
  template <size_t N>
  inline void validate(boost::any &out,
                       const std::vector<std::string> &values,
                       Blob<N> *,
                       long) {
    using namespace boost::program_options;
    check_first_occurrence(out);
    auto &value{get_single_string(values)};
    if (auto _bytes{Blob<N>::fromHex(value)}) {
      out = _bytes.value();
      return;
    }
    boost::throw_exception(invalid_option_value{value});
  }
}  // namespace fc::common

namespace libp2p::peer {
  inline void validate(boost::any &out,
                       const std::vector<std::string> &values,
                       PeerInfo *,
                       long) {
    using namespace boost::program_options;
    check_first_occurrence(out);
    auto &value{get_single_string(values)};
    if (auto _address{multi::Multiaddress::create(value)}) {
      auto &address{_address.value()};
      if (auto base58{address.getPeerId()}) {
        if (auto _id{PeerId::fromBase58(*base58)}) {
          out = PeerInfo{_id.value(), {address}};
          return;
        }
      }
    }
    boost::throw_exception(invalid_option_value{value});
  }
}  // namespace libp2p::peer

namespace fc::node {
  using config::configProfile;

  spdlog::level::level_enum getLogLevel(char level) {
    switch (level) {
      case 'e':
        return spdlog::level::err;
      case 'w':
        return spdlog::level::warn;
      case 'd':
        return spdlog::level::debug;
      case 't':
        return spdlog::level::trace;
    }
    return spdlog::level::info;
  }

  Config Config::read(int argc, char **argv) {
    Config config;
    struct {
      char log_level;
      boost::optional<boost::filesystem::path> copy_genesis, copy_config;
    } raw;
    namespace po = boost::program_options;
    po::options_description desc("Fuhon node options");
    auto option{desc.add_options()};
    option("help,h", "print usage message");
    option("repo", po::value(&config.repo_path)->required());
    option("config", po::value(&raw.copy_config), "copy config from file");
    option("genesis", po::value(&raw.copy_genesis), "copy genesis from file");
    option("api", po::value(&config.api_port)->default_value(1234), "API port");
    option("port,p", po::value(&config.port), "port to listen to");
    option("bootstrap,b",
           po::value(&config.bootstrap_list)->composing(),
           "remote bootstrap peer uri to connect to");
    option("log,l",
           po::value(&raw.log_level)->default_value('i'),
           "log level, [e,w,i,d,t]");
    option("import-snapshot", po::value(&config.snapshot));
    option("import-key",
           po::value(&config.wallet_default_key_path),
           "on first run, imports a default key from a given file. The key "
           "must be a BLS private key.");

    po::options_description drand_desc("Drand server options");
    auto drand_option{drand_desc.add_options()};
    drand_option("drand-server",
                 po::value(&config.drand_servers)->composing(),
                 "drand server uri");
    drand_option("drand-pubkey",
                 po::value(&config.drand_bls_pubkey),
                 "drand public key (bls)");
    drand_option("drand-genesis-time",
                 po::value(&config.drand_genesis),
                 "drand genesis time (seconds)");
    drand_option("drand-period",
                 po::value(&config.drand_period),
                 "drand period (seconds)");
    desc.add(drand_desc);

    desc.add(configProfile());
    primitives::address::configCurrentNetwork(option);

    po::variables_map vm;
    po::store(parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help") != 0) {
      std::cerr << desc << std::endl;
      exit(EXIT_SUCCESS);
    }
    boost::filesystem::create_directories(config.repo_path);
    std::ofstream{config.join(".pid")} << getpid();
    auto config_path{config.join("config.cfg")};
    if (raw.copy_config) {
      boost::filesystem::copy_file(
          *raw.copy_config,
          config_path,
          boost::filesystem::copy_option::overwrite_if_exists);
    }
    if (raw.copy_genesis) {
      boost::filesystem::copy_file(
          *raw.copy_genesis,
          config.genesisCar(),
          boost::filesystem::copy_option::overwrite_if_exists);
    }
    std::ifstream config_file{config_path};
    if (config_file.good()) {
      po::store(po::parse_config_file(config_file, desc), vm);
      po::notify(vm);
    }

    config.log_level = getLogLevel(raw.log_level);
    spdlog::set_level(config.log_level);

    return config;
  }

  std::string Config::join(const std::string &path) const {
    return (repo_path / path).string();
  }

  std::string Config::genesisCar() const {
    return join("genesis.car");
  }

  Multiaddress Config::p2pListenAddress() const {
    return libp2p::multi::Multiaddress::create(
               fmt::format("/ip4/0.0.0.0/tcp/{}", port))
        .value();
  }

  const std::string &Config::localIp() const {
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
}  // namespace fc::node
