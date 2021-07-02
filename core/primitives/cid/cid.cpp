/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/cid.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include "codec/uvarint.hpp"
#include "crypto/blake2/blake2b160.hpp"

using libp2p::multi::HashType;
using libp2p::multi::Multihash;

namespace fc {

  CID::CID() : ContentIdentifier({}, {}, Multihash::create({}, {}).value()) {}

  CID::CID(const ContentIdentifier &cid) : ContentIdentifier(cid) {}

  CID::CID(ContentIdentifier &&cid) noexcept
      : ContentIdentifier(
          cid.version, cid.content_type, std::move(cid.content_address)) {}

  CID::CID(Version version, Multicodec content_type, Multihash content_address)
      : ContentIdentifier(version, content_type, std::move(content_address)) {}

  CID::CID(const CbCid &cid)
      : CID{CID::Version::V1,
            CID::Multicodec::DAG_CBOR,
            Multihash::create(HashType::blake2b_256, cid).value()} {}

  CID::CID(const ActorCodeCid &cid)
      : CID{CID::Version::V1,
            CID::Multicodec::RAW,
            Multihash::create(HashType::identity, common::span::cbytes(cid))
                .value()} {}

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

  CidPrefix CID::getPrefix() const {
    return CidPrefix{
        .version = static_cast<uint64_t>(version),
        .codec = static_cast<uint64_t>(content_type),
        .mh_type = content_address.getType(),
        .mh_length = static_cast<int>(content_address.getHash().size())};
  }

  outcome::result<std::string> CID::toString() const {
    return libp2p::multi::ContentIdentifierCodec::toString(*this);
  }

  outcome::result<std::vector<uint8_t>> CID::toBytes() const {
    return libp2p::multi::ContentIdentifierCodec::encode(*this);
  }

  outcome::result<CID> CID::fromString(const std::string &str) {
    OUTCOME_TRY(cid, libp2p::multi::ContentIdentifierCodec::fromString(str));
    return CID{std::move(cid)};
  }

  outcome::result<CID> CID::fromBytes(gsl::span<const uint8_t> input) {
    OUTCOME_TRY(cid, libp2p::multi::ContentIdentifierCodec::decode(input));
    return CID{std::move(cid)};
  }

  outcome::result<CID> CID::read(gsl::span<const uint8_t> &input, bool prefix) {
    using Error = libp2p::multi::ContentIdentifierCodec::DecodeError;
    CID cid;
    if (input.size() >= 2 && HashType{input[0]} == HashType::sha256
        && input[1] == 32) {
      cid.version = Version::V0;
      cid.content_type = Multicodec::DAG_PB;
      if (!prefix && input.size() < 34) {
        return Multihash::Error::INCONSISTENT_LENGTH;
      }
    } else {
      if (!codec::uvarint::read(cid.version, input)) {
        return Error::EMPTY_VERSION;
      }
      if (cid.version != Version::V0 && cid.version != Version::V1) {
        return Error::RESERVED_VERSION;
      }
      if (!codec::uvarint::read(cid.content_type, input)) {
        return Error::EMPTY_MULTICODEC;
      }
    }

    HashType hash_type;
    if (!codec::uvarint::read(hash_type, input)) {
      return Multihash::Error::ZERO_INPUT_LENGTH;
    }
    size_t hash_size;
    if (!codec::uvarint::read(hash_size, input)) {
      return Multihash::Error::INPUT_TOO_SHORT;
    }
    gsl::span<const uint8_t> hash_span;
    if (prefix) {
      static const uint8_t empty[Multihash::kMaxHashLength]{};
      hash_span = gsl::make_span(empty, hash_size);
    } else {
      if (input.size() < static_cast<ptrdiff_t>(hash_size)) {
        return Multihash::Error::INCONSISTENT_LENGTH;
      }
      hash_span = input.subspan(0, hash_size);
      input = input.subspan(hash_size);
    }
    OUTCOME_TRY(hash, Multihash::create(hash_type, hash_span));
    cid.content_address = std::move(hash);
    return cid;
  }

  size_t hash_value(const CID &cid) {
    size_t seed{};
    boost::hash_combine(seed, cid.version);
    boost::hash_combine(seed, cid.content_type);
    boost::hash_combine(seed, cid.content_address.getType());
    auto hash{cid.content_address.getHash()};
    boost::hash_range(seed, hash.begin(), hash.end());
    return seed;
  }

  bool isCbor(const CID &cid) {
    return cid.version == CID::Version::V1
           && cid.content_type == CID::Multicodec::DAG_CBOR;
  }

  boost::optional<BytesIn> asIdentity(const CID &cid) {
    auto &mh{cid.content_address};
    if (mh.getType() == HashType::identity) {
      return mh.getHash();
    }
    return {};
  }

  boost::optional<ActorCodeCid> asActorCode(const CID &cid) {
    if (auto id{asIdentity(cid)}) {
      if (cid.content_type == CID::Multicodec::RAW) {
        return ActorCodeCid{common::span::bytestr(*id)};
      }
    }
    return boost::none;
  }

  boost::optional<CbCid> asBlake(const CID &cid) {
    auto &mh{cid.content_address};
    if (mh.getType() == HashType::blake2b_256) {
      if (auto hash{mh.getHash()}; hash.size() == CbCid::size()) {
        return CbCid{CbCid::fromSpan(hash).value()};
      }
    }
    return {};
  }
}  // namespace fc

namespace fc::common {
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes) {
    return CID{CbCid::hash(bytes)};
  }
}  // namespace fc::common
