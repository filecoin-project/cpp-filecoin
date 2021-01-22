/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BUILDER_CONFIG_HPP
#define CPP_FILECOIN_BUILDER_CONFIG_HPP

#include <spdlog/logger.h>
#include <boost/filesystem/path.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <libp2p/protocol/kademlia/config.hpp>

#include "crypto/bls/bls_types.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::node {

  struct Config {
    boost::filesystem::path repo_path;
    spdlog::level::level_enum log_level;
    libp2p::multi::Multiaddress listen_address;
    std::string local_ip;
    int port = -1;
    int api_port;
    std::string car_file_name;
    boost::optional<CID> genesis_cid;
    std::string network_name;
    std::vector<libp2p::peer::PeerInfo> bootstrap_list;
    libp2p::protocol::gossip::Config gossip_config;
    libp2p::protocol::kademlia::Config kademlia_config;

    // drand config
    std::vector<std::string> drand_servers;
    BlsPublicKey drand_bls_pubkey;
    int64_t drand_genesis = 0;
    int64_t drand_period = 0;
    unsigned beaconizer_cache_size = 100;

    Config();

    bool init(int argc, char *argv[]);

    std::string join(const std::string &path) const;
  };

}  // namespace fc::node

#endif  // CPP_FILECOIN_BUILDER_CONFIG_HPP
