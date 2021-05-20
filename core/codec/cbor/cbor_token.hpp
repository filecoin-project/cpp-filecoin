/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_common.hpp"
#include "codec/common.hpp"

namespace fc::codec::cbor {
  struct CborToken {
    enum Type : uint8_t {
      UINT,
      INT,
      BYTES,
      STR,
      LIST,
      MAP,
      CID,
      SPECIAL,
      INVALID,
    };

    Type type{Type::INVALID};
    uint64_t extra{};

    constexpr operator bool() const {
      return type != Type::INVALID;
    }

    constexpr bool isNull() const {
      return type == Type::SPECIAL && extra == kExtraNull;
    }
    constexpr std::optional<bool> asBool() const {
      if (type == Type::SPECIAL && extra == kExtraFalse) {
        return false;
      }
      if (type == Type::SPECIAL && extra == kExtraTrue) {
        return true;
      }
      return {};
    }
    constexpr std::optional<uint64_t> asUint() const {
      if (type == Type::UINT) {
        return extra;
      }
      return {};
    }
    constexpr std::optional<int64_t> asInt() const {
      if (type == Type::UINT) {
        return extra;
      }
      if (type == Type::INT) {
        return -static_cast<int64_t>(extra) - 1;
      }
      return {};
    }
    constexpr std::optional<size_t> bytesSize() const {
      if (type == Type::BYTES) {
        return extra;
      }
      return {};
    }
    constexpr std::optional<size_t> strSize() const {
      if (type == Type::STR) {
        return extra;
      }
      return {};
    }
    constexpr std::optional<size_t> cidSize() const {
      if (type == Type::CID) {
        return extra;
      }
      return {};
    }
    constexpr std::optional<size_t> listCount() const {
      if (type == Type::LIST) {
        return extra;
      }
      return {};
    }
    constexpr std::optional<size_t> mapCount() const {
      if (type == Type::MAP) {
        return extra;
      }
      return {};
    }
    constexpr size_t anySize() const {
      return type == Type::BYTES || type == Type::STR || type == Type::CID
                 ? extra
                 : 0;
    }
    constexpr size_t anyCount() const {
      return type == Type::LIST ? extra : type == Type::MAP ? 2 * extra : 0;
    }
  };

  struct CborTokenDecoder {
    CborToken value;
    size_t more{1};
    bool error{};
    bool tag{};
    bool cid{};

    inline void update(uint8_t byte) {
      using Type = CborToken::Type;
      assert(!error);
      assert(more);
      --more;
      if (cid) {
        return;
      }
      if (!value) {
        value.type = static_cast<Type>(byte >> 5);
        if (tag && value.type != Type::BYTES) {
          error = true;
          return;
        }
        byte &= 0x1F;
        if (byte < kExtraUint8) {
          value.extra = byte;
        } else {
          more = byte == kExtraUint8    ? 1
                 : byte == kExtraUint16 ? 2
                 : byte == kExtraUint32 ? 4
                 : byte == kExtraUint64 ? 8
                                        : 0;
          if (!more) {
            error = true;
            return;
          }
        }
      } else {
        value.extra = (value.extra << 8) | byte;
      }
      if (!more) {
        if (tag) {
          value.type = Type::CID;
          --value.extra;
          more = 1;
          cid = true;
        } else if (value.type == Type::CID) {
          if (value.extra != kExtraCid) {
            error = true;
            return;
          }
          value = {};
          more = 1;
          tag = true;
        }
      }
    }
  };

  struct CborNestedDecoder {
    CborTokenDecoder token;
    size_t more_count{};
    size_t more_size{};

    constexpr size_t more() const {
      return token.more + more_size + more_count;
    }
    constexpr bool error() const {
      return token.error;
    }
  };

  inline bool read(CborTokenDecoder &decoder, BytesIn &input) {
    while (decoder.more) {
      if (input.empty()) {
        return false;
      }
      decoder.update(input[0]);
      if (decoder.error) {
        return false;
      }
      fc::codec::read(input, 1);
    }
    return true;
  }

  inline bool read(CborNestedDecoder &decoder, BytesIn &input) {
    assert(!decoder.error());
    assert(decoder.more());
    while (true) {
      if (input.empty()) {
        return false;
      }
      if (decoder.more_size) {
        auto n{std::min<size_t>(decoder.more_size, input.size())};
        decoder.more_size -= n;
        fc::codec::read(input, n);
      } else {
        if (read(decoder.token, input)) {
          decoder.more_size = decoder.token.value.anySize();
          decoder.more_count += decoder.token.value.anyCount();
          if (decoder.more_count) {
            --decoder.more_count;
            decoder.token = {};
          }
        }
        if (decoder.error()) {
          return false;
        }
      }
      if (!decoder.more()) {
        return true;
      }
    }
  }

  inline CborToken read(CborToken &token, BytesIn &input) {
    token = {};
    CborTokenDecoder decoder;
    if (read(decoder, input)) {
      token = decoder.value;
    }
    return token;
  }

  /**
   * Reads CBOR nested item into nested and advances input to the next item.
   * @param[out] nested - read nested bytes
   * @param input - cvor bytes to read
   * @return true on success, otherwise false
   */
  inline bool readNested(BytesIn &nested, BytesIn &input) {
    auto ptr{input.data()};
    CborNestedDecoder decoder;
    if (read(decoder, input)) {
      nested = gsl::make_span(ptr, input.data());
      return true;
    }
    nested = {};
    return false;
  }

  inline bool findCid(BytesIn &cid, BytesIn &input) {
    while (!input.empty()) {
      CborToken token;
      if (!read(token, input)) {
        return false;
      }
      if (token.cidSize()) {
        return fc::codec::read(cid, input, *token.cidSize());
      }
      if (!fc::codec::read(input, token.anySize())) {
        return false;
      }
    }
    return false;
  }
}  // namespace fc::codec::cbor
