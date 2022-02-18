/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/rpc/client_setup.hpp"
#include "api/rpc/info.hpp"
#include "cli/cli.hpp"
#include "cli/validate/peer_info.hpp"
#include "common/git_commit_version/git_commit_version.hpp"
#include "common/libp2p/multi/multiaddress_fmt.hpp"

namespace fc::cli::_node {
  using libp2p::peer::PeerInfo;

  struct Node {
    struct Args {
      CLI_BOOL("version", "") version;
      CLI_DEFAULT("repo", "", boost::filesystem::path, ) repo;

      CLI_OPTS() {
        Opts opts;
        version(opts);
        repo(opts);
        return opts;
      }
    };
    CLI_RUN() {
      if (args.version) {
        fmt::print("fuhon-node-cli {}\n",
                   common::git_commit_version::getGitPrettyVersion());
        return;
      }
      throw ShowHelp{};
    }

    struct Api {
      std::shared_ptr<api::FullNodeApi> _;
      IoThread thread;
      std::shared_ptr<api::rpc::Client> wsc;

      Api(ArgsMap &argm) {
        const auto &args{argm.of<Node>()};
        const auto info{
            cliTry(api::rpc::loadInfo(*args.repo, "FULLNODE_API_INFO").o,
                   "api info is missing")};
        _ = std::make_shared<api::FullNodeApi>();
        wsc = std::make_shared<api::rpc::Client>(*thread.io);
        wsc->setup(*_);
        cliTry(wsc->connect(info.first, "/rpc/v1", info.second),
               "connecting to {}",
               info.first);
      }

      auto *operator->() const {
        return _.operator->();
      }
    };
  };

  struct Node_version : Empty {
    CLI_RUN() {
      Node::Api api{argm};
      const auto version{cliTry(api->Version())};
      fmt::print("Version: {}\n", version.version);
    }
  };
}  // namespace fc::cli::_node
