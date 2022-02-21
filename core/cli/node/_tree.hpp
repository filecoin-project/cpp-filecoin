/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/client.hpp"
#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/tree.hpp"

namespace fc::cli::_node {
  const auto _tree{tree<Node>(
      {{"net",
        tree<Group>({
            {"listen", tree<Node_net_listen>()},
        })},
       {"client",
        tree<Group>({
            {"retrieve", tree<Node_client_retrieve>()},
            {"import", tree<Node_client_importData>()},
            {"generate-car", tree<Node_client_generateCar>()},
            {"local", tree<Node_client_local>()},
        })},
       {"wallet",
        tree<Group>({
            {"new", tree<walletNew>()},
            {"list", tree<walletList>()},
            {"balance", tree<walletBalance>()},
            {"default", tree<walletDefault>()},
            {"set-default", tree<walletSetDefault>()},
            {"import", tree<walletImport>()},
            {"sign", tree<walletSign>()},
            {"verify", tree<walletVerify>()},
            {"delete",
             tree<walletDelete>()},
            {"market",
             tree<Group>({
                 {"withdraw", tree<walletWithdraw>()},
                 {"add", tree<walletAdd>()},
             })},
        })}})};
}  // namespace fc::cli::_node
