/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BUILDER_CONFIG_HPP
#define CPP_FILECOIN_BUILDER_CONFIG_HPP

#include <spdlog/logger.h>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>

#include "primitives/cid/cid.hpp"

namespace fc::node {

  struct Config {
    spdlog::level::level_enum log_level;
    std::string local_ip_address;
    int port;
    std::string network_name;
    std::string storage_car_file_name;
    CID genesis_cid;
    bool network_secio;
    libp2p::multi::Multiaddress listen_address;
    std::vector<libp2p::peer::PeerInfo> bootstrap_list;
    libp2p::protocol::gossip::Config gossip_config;

    Config();

    bool init(const std::string &config_file, int argc, char *argv[]);
  };

}  // namespace fc::node

#endif  // CPP_FILECOIN_BUILDER_CONFIG_HPP
