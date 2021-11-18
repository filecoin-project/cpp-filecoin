/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/protocol/gossip/gossip.hpp>
#include <libp2p/protocol/kademlia/config.hpp>

#include "common/logger.hpp"
#include "crypto/bls/bls_types.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::node {
  using libp2p::multi::Multiaddress;

  struct Config {
    boost::filesystem::path repo_path;
    spdlog::level::level_enum log_level;
    int port = 2000;
    int api_port;
    std::string api_ip;
    boost::optional<std::string> snapshot;
    boost::optional<CID> genesis_cid;
    boost::optional<std::string> network_name;
    std::vector<libp2p::peer::PeerInfo> bootstrap_list;
    libp2p::protocol::gossip::Config gossip_config;
    libp2p::protocol::kademlia::Config kademlia_config;

    // drand config
    std::vector<std::string> drand_servers;
    boost::optional<BlsPublicKey> drand_bls_pubkey;
    /** Drand genesis time in seconds */
    boost::optional<int64_t> drand_genesis;
    /** Drand round time in seconds */
    boost::optional<int64_t> drand_period;
    size_t beaconizer_cache_size = 100;

    /**
     * Adds libp2p connection in order to increase host score. Used for
     * debugging.
     */
    bool use_pubsub_workaround = true;

    /** Node default key path */
    boost::optional<std::string> wallet_default_key_path;

    size_t mpool_bls_cache_size{1000};

    static Config read(int argc, char *argv[]);

    std::string join(const std::string &path) const;
    std::string genesisCar() const;
    Multiaddress p2pListenAddress() const;
    const std::string &localIp() const;
  };
}  // namespace fc::node
