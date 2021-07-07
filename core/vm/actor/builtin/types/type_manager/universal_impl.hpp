/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/type_manager/universal.hpp"

#define UNIVERSAL_IMPL(T, V0, V2, V3, V4, V5)               \
  namespace fc::vm::actor::builtin::types {                 \
    template <>                                             \
    std::shared_ptr<T> Universal<T>::make(ActorVersion v) { \
      switch (v) {                                          \
        case ActorVersion::kVersion0:                       \
          return std::make_shared<V0>();                    \
        case ActorVersion::kVersion2:                       \
          return std::make_shared<V2>();                    \
        case ActorVersion::kVersion3:                       \
          return std::make_shared<V3>();                    \
        case ActorVersion::kVersion4:                       \
          return std::make_shared<V4>();                    \
        case ActorVersion::kVersion5:                       \
          return std::make_shared<V5>();                    \
      }                                                     \
    }                                                       \
    template <>                                             \
    codec::cbor::CborDecodeStream &Universal<T>::decode(    \
        codec::cbor::CborDecodeStream &s) {                 \
      switch (actor_version) {                              \
        case ActorVersion::kVersion0:                       \
          return s >> (V0 &)*object;                        \
        case ActorVersion::kVersion2:                       \
          return s >> (V2 &)*object;                        \
        case ActorVersion::kVersion3:                       \
          return s >> (V3 &)*object;                        \
        case ActorVersion::kVersion4:                       \
          return s >> (V4 &)*object;                        \
        case ActorVersion::kVersion5:                       \
          return s >> (V5 &)*object;                        \
      }                                                     \
    }                                                       \
    template <>                                             \
    codec::cbor::CborEncodeStream &Universal<T>::encode(    \
        codec::cbor::CborEncodeStream &s) const {           \
      switch (actor_version) {                              \
        case ActorVersion::kVersion0:                       \
          return s << (V0 &)*object;                        \
        case ActorVersion::kVersion2:                       \
          return s << (V2 &)*object;                        \
        case ActorVersion::kVersion3:                       \
          return s << (V3 &)*object;                        \
        case ActorVersion::kVersion4:                       \
          return s << (V4 &)*object;                        \
        case ActorVersion::kVersion5:                       \
          return s << (V5 &)*object;                        \
      }                                                     \
    }                                                       \
    template <>                                             \
    void Universal<T>::load(const IpldPtr &ipld) {          \
      switch (actor_version) {                              \
        case ActorVersion::kVersion0:                       \
          return cbor_blake::cbLoadT(ipld, (V0 &)*object);  \
        case ActorVersion::kVersion2:                       \
          return cbor_blake::cbLoadT(ipld, (V2 &)*object);  \
        case ActorVersion::kVersion3:                       \
          return cbor_blake::cbLoadT(ipld, (V3 &)*object);  \
        case ActorVersion::kVersion4:                       \
          return cbor_blake::cbLoadT(ipld, (V4 &)*object);  \
        case ActorVersion::kVersion5:                       \
          return cbor_blake::cbLoadT(ipld, (V5 &)*object);  \
      }                                                     \
    }                                                       \
    template <>                                             \
    outcome::result<void> Universal<T>::flush() {           \
      switch (actor_version) {                              \
        case ActorVersion::kVersion0:                       \
          return cbor_blake::cbFlushT((V0 &)*object);       \
        case ActorVersion::kVersion2:                       \
          return cbor_blake::cbFlushT((V2 &)*object);       \
        case ActorVersion::kVersion3:                       \
          return cbor_blake::cbFlushT((V3 &)*object);       \
        case ActorVersion::kVersion4:                       \
          return cbor_blake::cbFlushT((V4 &)*object);       \
        case ActorVersion::kVersion5:                       \
          return cbor_blake::cbFlushT((V5 &)*object);       \
      }                                                     \
    }                                                       \
  }
