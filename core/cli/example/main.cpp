/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/_tree.hpp"
#include "cli/run.hpp"

int main(int argc, const char *argv[]) {
  fc::cli::run("cli_example", fc::cli::_node::_tree, argc, argv);
}
