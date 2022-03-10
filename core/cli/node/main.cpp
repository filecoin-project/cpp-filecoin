/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/client.hpp"
#include "cli/node/net.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE) \
  { NAME, tree<TYPE>() }
#define CMDS(NAME, TYPE) NAME, tree<TYPE>
#define GROUP(NAME) CMDS(NAME, Group)

namespace fc::cli::_node {
  const auto _tree{tree<Node>({
      {GROUP("net")({
          CMD("connect", Node_net_connect),
          CMD("listen", Node_net_listen),
          CMD("peers", Node_net_peers),
      })},
      {GROUP("client")({
          CMD("retrieve", Node_client_retrieve),
          CMD("import", Node_client_importData),
          CMD("deal", Node_client_deal),
          CMD("generate-car", Node_client_generateCar),
          CMD("local", Node_client_local),
          CMD("find", Node_client_find),
          CMD("list-retrievals", Node_client_listRetrievals),
          CMD("inspect-deal", Node_client_inspectDeal),
          CMD("deal-stats", Node_client_dealStats),
          CMD("list-deals", Node_client_listDeals),
          CMD("balances", Node_client_balances),
          CMD("get-deal", Node_client_getDeal),
          CMD("stat", Node_client_stat),
          CMD("list-transfers", Node_client_listTransfers),
          CMD("grant-datacap", Node_client_grantDatacap),
          CMD("check-client-datacap", Node_client_checkClientDataCap),
          CMD("list-notaries", Node_client_listNotaries),
          CMD("list-clients", Node_client_listClients),
          CMD("check-notary-datacap", Node_client_checkNotaryDataCap)
      })},
      CMD("version", Node_version),
  })};
}  // namespace fc::cli::_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::_node::_tree, argc, argv);
}
