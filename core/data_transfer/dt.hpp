/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>

#include "data_transfer/message.hpp"
#include "fwd.hpp"
#include "storage/ipfs/graphsync/graphsync.hpp"

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
  using libp2p::peer::PeerId;
  using PeerGsId = gsns::FullRequestId;

  using DtId = uint64_t;
  struct PeerDtId {
    PeerId peer{codec::cbor::kDefaultT<PeerId>()};
    DtId id;
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
      OkCb on_begin, on_end;
    };

    static gsns::Extension makeExt(const DataTransferMessage &msg);

    static std::shared_ptr<DataTransfer> make(std::shared_ptr<Host> host,
                                              std::shared_ptr<Graphsync> gs);

    void push(const PeerInfo &peer,
              const CID &root,
              IpldPtr ipld,
              std::string type,
              Buffer voucher,
              OkCb on_begin,
              OkCb on_end);
    void acceptPush(const PeerDtId &pdtid, const CID &root, OkCb on_end);
    void rejectPush(const PeerDtId &pdtid);
    PeerDtId pull(const PeerInfo &peer,
                  const CID &root,
                  Selector selector,
                  std::string type,
                  Buffer voucher,
                  OnData on_reply,
                  OnCid on_cid);
    void pullOut(const PeerDtId &pdtid, std::string type, Buffer voucher);
    void acceptPull(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    std::string type,
                    Buffer voucher);
    void rejectPull(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    std::string type,
                    boost::optional<CborRaw> voucher);

    void onMsg(const PeerId &peer, const DataTransferMessage &msg);
    void dtSend(const PeerId &peer, const DataTransferMessage &msg);
    void dtSend(const PeerInfo &peer, const DataTransferMessage &msg);

    std::shared_ptr<Host> host;
    std::shared_ptr<Graphsync> gs;
    std::map<std::string, OnPush> on_push;
    std::map<std::string, OnPull> on_pull;
    std::unordered_map<PeerDtId, OnData> pulling_out;
    std::unordered_map<PeerDtId, OnData> pulling_in;
    std::unordered_map<PeerDtId, PushingOut> pushing_out;
    DtId next_dtid;

    LIBP2P_METRICS_INSTANCE_COUNT(fc::data_transfer::DataTransfer);
  };
}  // namespace fc::data_transfer
