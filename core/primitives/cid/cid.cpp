/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/cid/cid.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/uvarint.hpp>
#include "codec/uvarint.hpp"
#include "crypto/blake2/blake2b160.hpp"

using libp2p::multi::HashType;
using libp2p::multi::Multihash;

namespace fc {
  using libp2p::multi::UVarint;

  CID::CID() : ContentIdentifier({}, {}, Multihash::create({}, {}).value()) {}

  CID::CID(const ContentIdentifier &cid) : ContentIdentifier(cid) {}

  CID::CID(ContentIdentifier &&cid) noexcept
      : ContentIdentifier(
          cid.version, cid.content_type, std::move(cid.content_address)) {}

  CID::CID(Version version, Multicodec content_type, Multihash content_address)
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

  outcome::result<std::vector<uint8_t>> CID::getPrefix() const {
    std::vector<uint8_t> prefix;
    auto version_encoded = UVarint(static_cast<uint64_t>(version)).toVector();
    prefix.insert(prefix.end(), version_encoded.begin(), version_encoded.end());
    auto code_encoded = UVarint(static_cast<uint64_t>(content_type)).toVector();
    prefix.insert(prefix.end(), code_encoded.begin(), code_encoded.end());
    auto type_encoded = UVarint(content_address.getType()).toVector();
    prefix.insert(prefix.end(), type_encoded.begin(), type_encoded.end());
    auto size_encoded =
        UVarint(static_cast<uint64_t>(content_address.getHash().size()))
            .toVector();
    prefix.insert(prefix.end(), size_encoded.begin(), size_encoded.end());
    return std::move(prefix);
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
      OUTCOME_TRY(version,
                  codec::uvarint::read<Error::EMPTY_VERSION, Version>(input));
      if (version != Version::V0 && version != Version::V1) {
        return Error::RESERVED_VERSION;
      }
      cid.version = version;
      OUTCOME_TRY(
          codec,
          codec::uvarint::read<Error::EMPTY_MULTICODEC, Multicodec>(input));
      cid.content_type = codec;
    }

    OUTCOME_TRY(
        hash_type,
        codec::uvarint::read<Multihash::Error::ZERO_INPUT_LENGTH, HashType>(
            input));
    OUTCOME_TRY(hash_size,
                codec::uvarint::read<Multihash::Error::INPUT_TOO_SHORT>(input));
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
}  // namespace fc

namespace fc::common {
  outcome::result<CID> getCidOf(gsl::span<const uint8_t> bytes) {
    auto hash_raw = crypto::blake2b::blake2b_256(bytes);
    OUTCOME_TRY(hash, Multihash::create(HashType::blake2b_256, hash_raw));
    return CID(CID::Version::V1, CID::Multicodec::DAG_CBOR, hash);
  }
}  // namespace fc::common
