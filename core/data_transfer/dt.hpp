/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/libp2p/stream_open_queue.hpp"
#include "data_transfer/message.hpp"
#include "fwd.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"
#include "storage/ipld/traverser.hpp"

namespace fc::data_transfer {
  struct PeerDtId;
}  // namespace fc::data_transfer

namespace std {
  template <>
  struct hash<fc::data_transfer::PeerDtId> {
    size_t operator()(const fc::data_transfer::PeerDtId &pdtid) const;
  };
}  // namespace std

namespace fc::data_transfer {
  namespace gsns = storage::ipfs::graphsync;

  using gsns::Graphsync;
  using libp2p::Host;
  using libp2p::connection::StreamOpenQueue;
  using libp2p::peer::PeerId;
  using storage::ipld::traverser::Traverser;
  using PeerGsId = gsns::FullRequestId;

  using DtId = uint64_t;
  struct PeerDtId {
    PeerId peer{codec::cbor::kDefaultT<PeerId>()};
    DtId id{};
  };
  inline auto operator==(const PeerDtId &l, const PeerDtId &r) {
    return l.peer == r.peer && l.id == r.id;
  }

  struct DataTransfer {
    static inline const std::string kProtocol{"/fil/datatransfer/1.0.0"};
    static inline const std::string kExtension{"fil/data-transfer"};

    using OkCb = std::function<void(bool)>;
    using OnCid = std::function<void(const CID &)>;
    using OnData = std::function<void(const std::string &, BytesIn)>;
    using OnPush = std::function<void(
        const PeerDtId &, const CID &, const std::string &, BytesIn)>;
    using OnPull = std::function<void(
        const PeerDtId &, const PeerGsId &, const std::string &, BytesIn)>;

    struct PushingOut {
      CID root;
      IpldPtr ipld;
      OkCb on_begin;
      OkCb on_end;
      boost::optional<Traverser> traverser;
    };

    static gsns::Extension makeExt(const DataTransferMessage &msg);

    static std::shared_ptr<DataTransfer> make(
        std::shared_ptr<Host> host, const std::shared_ptr<Graphsync> &gs);

    void push(const PeerInfo &peer,
              const CID &root,
              IpldPtr ipld,
              std::string type,
              Bytes voucher,
              OkCb on_begin,
              OkCb on_end);
    void acceptPush(const PeerDtId &pdtid, const CID &root, OkCb on_end);
    void rejectPush(const PeerDtId &pdtid) const;
    PeerDtId pull(const PeerInfo &peer,
                  const CID &root,
                  Selector selector,
                  std::string type,
                  Bytes voucher,
                  OnData on_reply,
                  OnCid on_cid);
    void pullOut(const PeerDtId &pdtid, std::string type, Bytes voucher) const;
    void acceptPull(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    std::string type,
                    Bytes voucher) const;
    void rejectPull(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    std::string type,
                    boost::optional<CborRaw> voucher) const;

    void onMsg(const PeerId &peer, const DataTransferMessage &msg);
    void dtSend(const PeerId &peer, const DataTransferMessage &msg) const;
    void dtSend(const PeerInfo &peer, const DataTransferMessage &msg) const;

    std::shared_ptr<Host> host;
    std::shared_ptr<StreamOpenQueue> streams;
    std::shared_ptr<Graphsync> gs;
    std::map<std::string, OnPush> on_push;
    std::map<std::string, OnPull> on_pull;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::unordered_map<PeerDtId, OnData> pulling_out;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::unordered_map<PeerDtId, OnData> pulling_in;
    // TODO(turuslan): FIL-420 check cache memory usage
    std::unordered_map<PeerDtId, PushingOut> pushing_out;
    DtId next_dtid = 0;
  };
}  // namespace fc::data_transfer
