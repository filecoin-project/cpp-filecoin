/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/node.hpp"

namespace fc::cli::_node {
  struct Node_client_retrieve {
    struct Args {
      CLI_OPTIONAL("from", "", Address) from;
      CLI_OPTIONAL("provider", "", Address) provider;
      CLI_OPTIONAL("pieceCid", "", CID) piece_cid;
      CLI_OPTIONAL("maxPrice", "", AttoFil) max_price;
      CLI_OPTIONAL("data-selector", "", std::string) data_selector;
      CLI_BOOL("car", "") car;
      CLI_BOOL("allow-local", "") allow_local;
      CLI_BOOL("car-export-merkle-proof", "") car_export_merkle_proof;

      CLI_OPTS() {
        Opts opts;
        from(opts);
        provider(opts);
        piece_cid(opts);
        allow_local(opts);
        car(opts);
        max_price(opts);
        data_selector(opts);
        car_export_merkle_proof(opts);
        return opts;
      }
    };
    CLI_RUN() {
      auto data_cid{cliArgv<CID>(argv, 0, "dataCid")};
      auto path{cliArgv<boost::filesystem::path>(argv, 1, "path")};
      if (args.max_price) {
        fmt::print("max price is {}fil ({}attofil)\n",
                   args.max_price->fil,
                   args.max_price->atto());
      }
      fmt::print("retrieving {} to {} {}\n",
                 data_cid,
                 args.car ? "car" : "file",
                 path);
      fmt::print("TODO: implement\n");
    }
  };

  struct Node_client_importData {
    struct Args {
      CLI_BOOL("car", "") car;

      CLI_OPTS() {
        Opts opts;
        car(opts);
        return opts;
      }
    };
    CLI_RUN() {
      auto path{cliArgv<boost::filesystem::path>(argv, 0, "inputPath")};
      fmt::print("TODO: implement\n");
    }
  };

  struct Node_client_generateCar : Empty {
    CLI_RUN() {
      auto input_path{cliArgv<boost::filesystem::path>(argv, 0, "inputPath")};
      auto output_path{cliArgv<boost::filesystem::path>(argv, 1, "outputPath")};
      fmt::print("TODO: implement\n");
    }
  };

  struct Node_client_local : Empty {
    CLI_RUN() {
      fmt::print("TODO: implement\n");
    }
  };
}  // namespace fc::cli::_node
