//
// Created by Ruslan Gilvanov  on 12.02.2022.

#pragma once
#include "cli/node/node.hpp"

namespace fc::cli::_node {

  struct clientRetrive {
    struct Args {
      bool car{};
      bool export_merkle_root{};
      std::string data_selector;

      CLI_OPTS() {
        Opts opts;
        auto option{opts.add_options()};
        option("car",
               po::bool_switch(&car),
               "Export to a car file instead of a regular file");
        option("data-selector",
               po::value(&data_selector),
               "IPLD datamodel text-path selector, or IPLD json selector");
        option("car-export-merkle-proof",
               po::bool_switch(&export_merkle_root),
               "(requires --data-selector and --car) Export data-selector "
               "merkle proof");
        return opts;
      }
    };

    CLI_RUN() {
      std::cout << args.data_selector << "\n";
    }
  };

}  // namespace fc::cli::_node
