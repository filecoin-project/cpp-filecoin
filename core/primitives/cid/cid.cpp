/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/primitives/cid/cid.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/uvarint.hpp>
#include "filecoin/crypto/blake2/blake2b160.hpp"

namespace fc {
  CID::CID()
      : ContentIdentifier(
          {}, {}, libp2p::multi::Multihash::create({}, {}).value()) {}

  CID::CID(const ContentIdentifier &cid) : ContentIdentifier(cid) {}

  CID::CID(ContentIdentifier &&cid) noexcept
      : ContentIdentifier(
          cid.version, cid.content_type, std::move(cid.content_address)) {}

  CID::CID(Version version,
           libp2p::multi::MulticodecType::Code content_type,
           libp2p::multi::Multihash content_address)
      : ContentIdentifier(version, content_type, std::move(content_address)) {}

  CID &CID::operator=(CID &&cid) noexcept {
    version = cid.version;
    content_type = cid.content_type;
    content_address = std::move(cid.content_address);
    return *this;
  }

  CID &CID::operator=(const ContentIdentifier &cid) {
    version = cid.version;
    content_type = cid.content_type;
    content_address = cid.content_address;
    return *this;
  }

  CID::CID(CID &&cid) noexcept
      : ContentIdentifier(
          cid.version, cid.content_type, std::move(cid.content_address)) {}

  CID &CID::operator=(ContentIdentifier &&cid) {
    version = cid.version;
    content_type = cid.content_type;
    content_address = std::move(cid.content_address);
    return *this;
  }

  outcome::result<std::string> CID::toString() const {
    return libp2p::multi::ContentIdentifierCodec::toString(*this);
  }

  outcome::result<std::vector<uint8_t>> CID::toBytes() const {
    return libp2p::multi::ContentIdentifierCodec::encode(*this);
  }

}  // namespace fc

namespace fc::common {
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes) {
    auto hash_raw = crypto::blake2b::blake2b_256(bytes);
    OUTCOME_TRY(hash,
                libp2p::multi::Multihash::create(
                    libp2p::multi::HashType::blake2b_256, hash_raw));
    return CID(CID::Version::V1, libp2p::multi::MulticodecType::DAG_CBOR, hash);
  }
}  // namespace fc::common
