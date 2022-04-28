/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "primitives/address/address.hpp"
#include "primitives/sector/sector.hpp"
#include "primitives/types.hpp"

namespace fc::vm::actor::builtin::types::storage_power {
  using libp2p::multi::Multiaddress;
  using primitives::address::Address;
  using primitives::sector::RegisteredPoStProof;

  struct CreateMinerParams {
    Address owner;
    Address worker;
    RegisteredPoStProof window_post_proof_type;
    Bytes peer_id;
    std::vector<Multiaddress> multiaddresses;

    inline bool operator==(const CreateMinerParams &other) const {
      return owner == other.owner && worker == other.worker
             && window_post_proof_type == other.window_post_proof_type
             && peer_id == other.peer_id
             && multiaddresses == other.multiaddresses;
    }

    inline bool operator!=(const CreateMinerParams &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(CreateMinerParams,
             owner,
             worker,
             window_post_proof_type,
             peer_id,
             multiaddresses)

}  // namespace fc::vm::actor::builtin::types::storage_power
