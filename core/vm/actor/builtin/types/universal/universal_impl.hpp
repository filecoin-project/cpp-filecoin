/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/builtin/types/universal/universal.hpp"

#define UNIVERSAL_IMPL(T, V0, V2, V3, V4, V5, V6, V7)       \
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
        case ActorVersion::kVersion6:                       \
          return std::make_shared<V6>();                    \
        case ActorVersion::kVersion7:                       \
          return std::make_shared<V7>();                    \
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
        case ActorVersion::kVersion6:                       \
          return s >> (V6 &)*object;                        \
        case ActorVersion::kVersion7:                       \
          return s >> (V7 &)*object;                        \
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
        case ActorVersion::kVersion6:                       \
          return s << (V6 &)*object;                        \
        case ActorVersion::kVersion7:                       \
          return s << (V7 &)*object;                        \
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
        case ActorVersion::kVersion6:                       \
          return cbor_blake::cbLoadT(ipld, (V6 &)*object);  \
        case ActorVersion::kVersion7:                       \
          return cbor_blake::cbLoadT(ipld, (V7 &)*object);  \
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
        case ActorVersion::kVersion6:                       \
          return cbor_blake::cbFlushT((V6 &)*object);       \
        case ActorVersion::kVersion7:                       \
          return cbor_blake::cbFlushT((V7 &)*object);       \
      }                                                     \
    }                                                       \
    template <>                                             \
    std::shared_ptr<T> Universal<T>::copy_object() const {  \
      switch (actor_version) {                              \
        case ActorVersion::kVersion0:                       \
          return std::make_shared<V0>((V0 &)*object);       \
        case ActorVersion::kVersion2:                       \
          return std::make_shared<V2>((V2 &)*object);       \
        case ActorVersion::kVersion3:                       \
          return std::make_shared<V3>((V3 &)*object);       \
        case ActorVersion::kVersion4:                       \
          return std::make_shared<V4>((V4 &)*object);       \
        case ActorVersion::kVersion5:                       \
          return std::make_shared<V5>((V5 &)*object);       \
        case ActorVersion::kVersion6:                       \
          return std::make_shared<V6>((V6 &)*object);       \
        case ActorVersion::kVersion7:                       \
          return std::make_shared<V7>((V7 &)*object);       \
      }                                                     \
    }                                                       \
  }
