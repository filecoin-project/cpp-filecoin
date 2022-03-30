/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "codec/json/coding.hpp"
#include "common/libp2p/multi/json_multiaddress.hpp"
#include "common/libp2p/peer/cbor_peer_info.hpp"
#include "common/libp2p/peer/json_peer_id.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::api {
  using libp2p::multi::Multiaddress;
  using libp2p::peer::PeerId;
  using primitives::SectorSize;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;

  struct MinerInfo {
    Address owner;
    Address worker;
    std::vector<Address> control;
    Bytes peer_id;
    std::vector<Multiaddress> multiaddrs;
    RegisteredPoStProof window_post_proof_type{RegisteredPoStProof::kUndefined};
    SectorSize sector_size{};
    uint64_t window_post_partition_sectors{};
  };

  JSON_ENCODE(MinerInfo) {
    using fc::codec::json::Set;
    using fc::codec::json::Value;
    Value j{rapidjson::kObjectType};
    Set(j, "Owner", v.owner, allocator);
    Set(j, "Worker", v.worker, allocator);
    Set(j, "ControlAddresses", v.control, allocator);
    boost::optional<std::string> peer_id;
    if (!v.peer_id.empty()) {
      OUTCOME_EXCEPT(peer, PeerId::fromBytes(v.peer_id));
      peer_id = peer.toBase58();
    }
    Set(j, "PeerId", peer_id, allocator);
    Set(j, "Multiaddrs", v.multiaddrs, allocator);
    Set(j, "WindowPoStProofType", v.window_post_proof_type, allocator);
    Set(j, "SectorSize", v.sector_size, allocator);
    Set(j,
        "WindowPoStPartitionSectors",
        v.window_post_partition_sectors,
        allocator);
    return j;
  }

  JSON_DECODE(MinerInfo) {
    using fc::codec::json::Get;
    Get(j, "Owner", v.owner);
    Get(j, "Worker", v.worker);
    Get(j, "ControlAddresses", v.control);
    boost::optional<PeerId> peer_id;
    Get(j, "PeerId", peer_id);
    if (peer_id) {
      v.peer_id = peer_id->toVector();
    } else {
      v.peer_id.clear();
    }
    Get(j, "Multiaddrs", v.multiaddrs);
    Get(j, "WindowPoStProofType", v.window_post_proof_type);
    Get(j, "SectorSize", v.sector_size);
    Get(j, "WindowPoStPartitionSectors", v.window_post_partition_sectors);
  }
}  // namespace fc::api