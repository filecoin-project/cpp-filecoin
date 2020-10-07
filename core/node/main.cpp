/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "api/make.hpp"
#include "clock/impl/utc_clock_impl.hpp"

namespace fc {
  struct Config {
    boost::filesystem::path repo_path;
    int p2p_port, api_port;

    auto join(const std::string &path) const {
      return (repo_path / path).string();
    }
  };

  outcome::result<Config> readConfig(int argc, char **argv) {
    namespace po = boost::program_options;
    Config config;
    std::string repo_path;
    po::options_description desc("Fuhon node options");
    auto option{desc.add_options()};
    option("repo", po::value(&repo_path)->required());
    option("p2p_port", po::value(&config.p2p_port)->default_value(3020));
    option("api_port", po::value(&config.api_port)->default_value(3021));
    po::variables_map vm;
    po::store(parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    config.repo_path = repo_path;
    assert(boost::filesystem::exists(config.repo_path));
    std::ifstream config_file{config.join("config.cfg")};
    if (config_file.good()) {
      po::store(po::parse_config_file(config_file, desc), vm);
      po::notify(vm);
    }
    return config;
  }

  outcome::result<void> main(Config &config) {
    auto clock{std::make_shared<clock::UTCClockImpl>()};

    OUTCOME_TRY(p2p_listen,
                Multiaddress::create(
                    fmt::format("/ip4/127.0.0.1/tcp/{}", config.p2p_port)));

    // TODO: hostname, listen 0.0.0.0
    std::ofstream{config.join("api")}
        << fmt::format("/ip4/127.0.0.1/tcp/{}/http", config.api_port);
    std::ofstream{config.join("token")} << "token";

    return outcome::success();
  }
}  // namespace fc

int main(int argc, char **argv) {
  OUTCOME_EXCEPT(config, fc::readConfig(argc, argv));
  OUTCOME_EXCEPT(fc::main(config));
}
