/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/address/address_codec.hpp"

#include <libp2p/multi/multibase_codec/codecs/base32.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <stdexcept>

#include "common/visitor.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "crypto/bls/bls_types.hpp"
#include "primitives/address/address.hpp"

namespace fc::primitives::address {
  constexpr size_t kChecksumSize{4};

  using libp2p::multi::UVarint;

  Bytes encode(const Address &address) noexcept {
    Bytes res;
    res.push_back(address.getProtocol());
    visit_in_place(
        address.data,
        [&](ActorId v) { append(res, (UVarint{v}.toVector())); },
        [&](const auto &v) { append(res, v); });
    return res;
  }

  outcome::result<Address> decode(Protocol protocol, BytesIn payload) {
    switch (protocol) {
      case Protocol::ID: {
        if (auto value{UVarint::create(payload)}) {
          return Address{value->toUInt64()};
        }
        break;
      }
      case Protocol::SECP256K1: {
        if (auto hash{Secp256k1PublicKeyHash::fromSpan(payload)}) {
          return Address{Secp256k1PublicKeyHash{hash.value()}};
        }
        break;
      }
      case Protocol::ACTOR: {
        if (auto hash{ActorExecHash::fromSpan(payload)}) {
          return Address{ActorExecHash{hash.value()}};
        }
        break;
      }
      case Protocol::BLS: {
        if (auto key{BLSPublicKeyHash::fromSpan(payload)}) {
          return Address{BLSPublicKeyHash{key.value()}};
        }
        break;
      }
      default:
        return AddressError::kUnknownProtocol;
    }
    return AddressError::kInvalidPayload;
  }

  outcome::result<Address> decode(BytesIn v) {
    if (v.size() < 1) {
      return AddressError::kInvalidPayload;
    }
    return decode(Protocol{v[0]}, v.subspan(1));
  }

  std::string encodeToString(const Address &address) {
    std::string res;
    res.push_back(currentNetwork == Network::TESTNET ? 't' : 'f');
    res.push_back('0' + address.getProtocol());
    if (address.isId()) {
      res.append(std::to_string(address.getId()));
      return res;
    }
    Bytes buffer;
    visit_in_place(
        address.data,
        [](ActorId v) {},
        [&](const auto &v) { append(buffer, v); });
    append(buffer, checksum(address));
    return res.append(libp2p::multi::detail::encodeBase32Lower(buffer));
  }

  outcome::result<Address> decodeFromString(const std::string &s) {
    if (s.size() < 3) {
      return AddressError::kInvalidPayload;
    }
    if (s[0] != 'f' && s[0] != 't') {
      return AddressError::kUnknownNetwork;
    }
    int protocol = int(s[1]) - int('0');
    if (protocol < Protocol::ID || protocol > Protocol::BLS) {
      return AddressError::kUnknownProtocol;
    }
    auto tail{std::string_view{s}.substr(2)};
    if (protocol == Protocol::ID) {
      try {
        auto value{static_cast<ActorId>(std::stoul(std::string{tail}))};
        return Address{value};
      } catch (std::invalid_argument &e) {
        return AddressError::kInvalidPayload;
      } catch (std::out_of_range &e) {
        return AddressError::kInvalidPayload;
      }
    }
    OUTCOME_TRY(buffer, libp2p::multi::detail::decodeBase32Lower(tail));
    if (buffer.size() < kChecksumSize) {
      return AddressError::kInvalidPayload;
    }
    return decode(
        (Protocol)protocol,
        gsl::make_span(buffer).subspan(0, buffer.size() - kChecksumSize));
  }

  Bytes checksum(const Address &address) {
    Bytes res;
    if (!address.isId()) {
      res.resize(kChecksumSize);
      crypto::blake2b::hashn(res, encode(address));
    }
    return res;
  }
}  // namespace fc::primitives::address
