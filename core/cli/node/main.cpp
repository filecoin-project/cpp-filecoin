/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/client.hpp"
#include "cli/node/developer.hpp"
#include "cli/node/filplus.hpp"
#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE) \
  { NAME, tree<TYPE>() }
#define CMDS(NAME, TYPE) NAME, tree<TYPE>
#define GROUP(NAME) CMDS(NAME, Group)

namespace fc::cli::cli_node {
  const auto _tree{tree<Node>({
      {GROUP("net")({
          CMD("connect", Node_net_connect),
          CMD("listen", Node_net_listen),
          CMD("peers", Node_net_peers),
      })},
      {GROUP("filplus")({
          CMD("grant-datacap", Node_filplus_grantDatacap),
          CMD("list-notaries", Node_filplus_listNotaries),
          CMD("list-clients", Node_filplus_listClients),
          CMD("add-verifier", Node_filplus_addVerifier),
          CMD("check-client-datacap", Node_filplus_checkClientDataCap),
          CMD("check-notary-datacap", Node_filplus_checkNotaryDataCap),
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
      })},
      {GROUP("wallet")({
          CMD("new", Node_wallet_new),
          CMD("list", Node_wallet_list),
          CMD("add-balance", Node_wallet_addBalance),
          CMD("balance", Node_wallet_balance),
          CMD("default", Node_wallet_default),
          CMD("set-default", Node_wallet_setDefault),
          CMD("import", Node_wallet_import),
          CMD("sign", Node_wallet_sign),
          CMD("verify", Node_wallet_verify),
          CMD("delete", Node_wallet_delete),
          {GROUP("market")({
              CMD("add", Node_wallet_market_add),
          })},
      })},
      {GROUP("mpool")({
          CMD("pending", Node_mpool_pending),
          CMD("subscribe", Node_mpool_sub),
          CMD("find", Node_mpool_find),
      })},
      {GROUP("auth")({
          CMD("create-token", Node_auth_createToken),
          CMD("api-info", Node_auth_apiInfo),
      })},
      {GROUP("chain")({
          CMD("head", Node_chain_head),
          CMD("get-block", Node_chain_getBlock),
          CMD("read-object", Node_chain_readObject),
          CMD("get-message", Node_chain_getMessage),
          CMD("get", Node_chain_get),
          CMD("slash-consensus", Node_chain_slashConsensus),
          CMD("gas-price", Node_chain_estimateGasPrices),
      })},
      {GROUP("state")({
          CMD("miner-info", Node_state_stateMinerInfo),
          CMD("network-version", Node_state_networkVersion),
          {GROUP("market")({
              CMD("balance", Node_state_market_balance),
          })},
          CMD("sector", Node_state_sector),
          CMD("call", Node_state_call),
          CMD("search-msg", Node_state_searchMsg),
          CMD("wait-msg", Node_state_waitMsg),
          CMD("sector-size", Node_state_sectorSize),
          CMD("lookup", Node_state_lookup),
          CMD("get-actor", Node_state_getActor),
          CMD("list-actors", Node_state_listActors),
          CMD("list-miners", Node_state_listMiners),
          CMD("get-deal", Node_state_getDeal),
          CMD("active-sectors", Node_state_activeSectors),
          CMD("sectors", Node_state_sectors),
          CMD("power", Node_state_power),
      })},
      CMD("version", Node_version),
  })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
