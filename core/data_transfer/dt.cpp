/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "data_transfer/dt.hpp"

#include <libp2p/host/host.hpp>

#include "common/libp2p/cbor_stream.hpp"
#include "common/ptr.hpp"
#include "storage/ipfs/graphsync/extension_dedup.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

size_t std::hash<fc::data_transfer::PeerDtId>::operator()(
    const fc::data_transfer::PeerDtId &pdtid) const {
  size_t seed{};
  boost::hash_combine(seed, std::hash<libp2p::peer::PeerId>{}(pdtid.peer));
  boost::hash_combine(seed, pdtid.id);
  return seed;
}

namespace fc::data_transfer {
  constexpr size_t kStreamOpenMax{20};

  using common::libp2p::CborStream;
  using libp2p::protocol::Subscription;
  using storage::ipld::kAllSelector;

  void _read(const std::weak_ptr<DataTransfer> &_dt,
             const std::shared_ptr<CborStream> &s) {
    if (_dt.expired()) {
      return s->close();
    }
    s->read<DataTransferMessage>([_dt, s](auto _msg) {
      if (auto dt{_dt.lock()}) {
        if (_msg) {
          dt->onMsg(s->stream()->remotePeerId().value(), _msg.value());
          _read(_dt, s);
          return;
        }
      }
      s->close();
    });
  }

  gsns::Extension DataTransfer::makeExt(const DataTransferMessage &msg) {
    return {kExtension, Bytes{codec::cbor::encode(msg).value()}};
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  std::shared_ptr<DataTransfer> DataTransfer::make(
      std::shared_ptr<Host> host, const std::shared_ptr<Graphsync> &gs) {
    auto dt{std::make_shared<DataTransfer>()};
    dt->host = host;
    dt->streams = std::make_shared<StreamOpenQueue>(host, kStreamOpenMax);
    dt->gs = gs;
    gs->setRequestHandler(
        // NOLINTNEXTLINE(readability-function-cognitive-complexity)
        [_dt{weaken(dt)}](auto pgsid, auto req) {
          auto dt{_dt.lock()};
          if (!dt) {
            return;
          }
          auto ext{*gsns::Extension::find(kExtension, req.extensions)};
          OUTCOME_EXCEPT(msg, codec::cbor::decode<DataTransferMessage>(ext));
          PeerDtId pdtid{pgsid.peer, msg.dtid()};
          if (msg.is_request) {
            const auto &req{*msg.request};
            const auto it{dt->on_pull.find(req.voucher_type)};
            if (!req.is_pull || it == dt->on_pull.end()
                || dt->pulling_in.count(pdtid)) {
              return dt->rejectPull(pdtid, pgsid, {}, {});
            }
            it->second(pdtid, pgsid, req.voucher_type, req.voucher->b);
          } else {
            auto &res{*msg.response};
            auto it{dt->pushing_out.find(pdtid)};
            if (it != dt->pushing_out.end()) {
              auto &push{it->second};
              if (push.on_begin) {
                push.on_begin(res.is_accepted);
                push.on_begin = {};
                if (res.is_accepted) {
                  push.traverser.emplace(Traverser{
                      *push.ipld,
                      req.root_cid,
                      CborRaw{req.selector},
                      true,
                  });
                  dt->gs->postBlocks(
                      pgsid,
                      [=, &push](bool ok) -> boost::optional<gsns::Response> {
                        gsns::Response res;
                        if (ok) {
                          if (push.traverser->isCompleted()) {
                            res.status =
                                gsns::ResponseStatusCode::RS_FULL_CONTENT;
                            return res;
                          }
                          if (auto _cid{push.traverser->advance()}) {
                            auto &cid{_cid.value()};
                            if (auto _data{push.ipld->get(cid)}) {
                              res.status =
                                  gsns::ResponseStatusCode::RS_PARTIAL_RESPONSE;
                              res.data.emplace_back(gsns::Data{
                                  std::move(cid), std::move(_data.value())});
                              return res;
                            }
                          }
                        }
                        push.on_end(false);
                        dt->pushing_out.erase(it);
                        if (ok) {
                          res.status = gsns::ResponseStatusCode::RS_REJECTED;
                          return res;
                        }
                        return boost::none;
                      });
                  return;
                }
              } else {
                push.on_end(false);
              }
              dt->pushing_out.erase(it);
            }
            dt->gs->postResponse(
                pgsid, {gsns::ResponseStatusCode::RS_REJECTED, {}, {}});
          }
        },
        kExtension);
    host->setProtocolHandler(kProtocol, [_dt{weaken(dt)}](auto s) {
      if (auto dt{_dt.lock()}) {
        if (s->remotePeerId()) {
          return _read(_dt, std::make_shared<CborStream>(s));
        }
      }
      s->reset();
    });
    return dt;
  }

  void DataTransfer::push(const PeerInfo &peer,
                          const CID &root,
                          IpldPtr ipld,
                          std::string type,
                          Bytes voucher,
                          OkCb on_begin,
                          OkCb on_end) {
    auto dtid{next_dtid++};
    assert(on_begin);
    pushing_out.emplace(
        PeerDtId{peer.id, dtid},
        PushingOut{
            root, std::move(ipld), std::move(on_begin), std::move(on_end), {}});
    dtSend(peer,
           DataTransferMessage{DataTransferRequest{
               root,
               MessageType::kNewMessage,
               false,
               false,
               false,
               kAllSelector,
               CborRaw{std::move(voucher)},
               std::move(type),
               dtid,
           }});
  }

  void DataTransfer::acceptPush(const PeerDtId &pdtid,
                                const CID &root,
                                OkCb on_end) {
    auto sub{std::make_shared<Subscription>()};
    *sub = gs->makeRequest(
        {pdtid.peer, {}},
        root,
        kAllSelector.b,
        {makeExt(DataTransferMessage{DataTransferResponse{
            MessageType::kNewMessage,
            true,
            false,
            pdtid.id,
            {},
            {},
        }})},
        [this, pdtid, MOVE(on_end), sub](auto code, auto) {
          if (gsns::isTerminal(code)) {
            auto ok{code == gsns::ResponseStatusCode::RS_FULL_CONTENT};
            if (ok) {
              dtSend(pdtid.peer,
                     DataTransferMessage{DataTransferResponse{
                         MessageType::kCompleteMessage,
                         true,
                         false,
                         pdtid.id,
                         {},
                         {},
                     }});
            }
            on_end(ok);
          }
        });
  }

  void DataTransfer::rejectPush(const PeerDtId &pdtid) const {
    dtSend(pdtid.peer,
           DataTransferMessage{DataTransferResponse{
               MessageType::kCompleteMessage, false, false, pdtid.id, {}, {}}});
  }

  PeerDtId DataTransfer::pull(const PeerInfo &peer,
                              const CID &root,
                              Selector selector,
                              std::string type,
                              Bytes voucher,
                              OnData on_reply,
                              OnCid on_cid) {
    auto dtid{next_dtid++};
    PeerDtId pdtid{peer.id, dtid};
    pulling_out.emplace(pdtid, std::move(on_reply));
    auto sub{std::make_shared<Subscription>()};
    *sub = gs->makeRequest(
        peer,
        root,
        selector.b,
        {
            makeExt(DataTransferMessage{DataTransferRequest{
                root,
                MessageType::kNewMessage,
                false,
                false,
                true,
                selector,
                CborRaw{std::move(voucher)},
                std::move(type),
                dtid,
            }}),
            storage::ipfs::graphsync::extension::dedup::make(
                std::to_string(dtid)),
        },
        [this, peer, MOVE(on_cid), sub](auto, auto ext) {
          if (auto _ext{gsns::Extension::find(gsns::kResponseMetadataProtocol,
                                              ext)}) {
            OUTCOME_EXCEPT(meta,
                           codec::cbor::decode<gsns::ResponseMetadata>(*_ext));
            for (auto &p : meta) {
              on_cid(p.cid);
            }
          }
          if (auto _ext{gsns::Extension::find(kExtension, ext)}) {
            OUTCOME_EXCEPT(msg,
                           codec::cbor::decode<DataTransferMessage>(*_ext));
            onMsg(peer.id, msg);
          }
        });
    return pdtid;
  }

  void DataTransfer::pullOut(const PeerDtId &pdtid,
                             std::string type,
                             Bytes voucher) const {
    dtSend(pdtid.peer,
           DataTransferMessage{DataTransferRequest{
               {},
               MessageType::kVoucherMessage,
               false,
               false,
               true,
               {},
               CborRaw{std::move(voucher)},
               std::move(type),
               pdtid.id,
           }});
  }

  void DataTransfer::acceptPull(const PeerDtId &pdtid,
                                const PeerGsId &pgsid,
                                std::string type,
                                Bytes voucher) const {
    assert(pdtid.peer == pgsid.peer);
    gs->postResponse(
        pgsid,
        {
            gsns::ResponseStatusCode::RS_PARTIAL_RESPONSE,
            {DataTransfer::makeExt(DataTransferMessage{DataTransferResponse{
                data_transfer::MessageType::kNewMessage,
                true,
                false,
                pdtid.id,
                CborRaw{std::move(voucher)},
                std::move(type),
            }})},
            {},
        });
  }

  void DataTransfer::rejectPull(const PeerDtId &pdtid,
                                const PeerGsId &pgsid,
                                std::string type,
                                boost::optional<CborRaw> voucher) const {
    assert(pdtid.peer == pgsid.peer);
    gs->postResponse(pgsid,
                     {
                         gsns::ResponseStatusCode::RS_REJECTED,
                         {makeExt(DataTransferMessage{DataTransferResponse{
                             MessageType::kCompleteMessage,
                             false,
                             false,
                             pdtid.id,
                             std::move(voucher),
                             std::move(type),
                         }})},
                         {},
                     });
  }

  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  void DataTransfer::onMsg(const PeerId &peer, const DataTransferMessage &msg) {
    PeerDtId pdtid{peer, msg.dtid()};
    if (msg.is_request) {
      const auto &req{*msg.request};
      if (req.voucher) {
        if (req.base_cid) {
          auto it{on_push.find(req.voucher_type)};
          if (it != on_push.end()) {
            return it->second(
                pdtid, *req.base_cid, req.voucher_type, req.voucher->b);
          }
        } else {
          auto it{pulling_in.find(pdtid)};
          if (it != pulling_in.end()) {
            return it->second(req.voucher_type, req.voucher->b);
          }
        }
      }
      rejectPush(pdtid);
    } else {
      const auto &res{*msg.response};
      auto _pull{pulling_out.find(pdtid)};
      if (_pull != pulling_out.end()) {
        if (!res.voucher) {
          return;
        }
        _pull->second(res.voucher_type, res.voucher->b);
      } else {
        auto _push{pushing_out.find(pdtid)};
        if (_push != pushing_out.end()) {
          auto &push{_push->second};
          if (push.on_begin) {
            push.on_begin(false);
          } else {
            _push->second.on_end(true);
          }
          pushing_out.erase(_push);
        }
      }
    }
  }

  void DataTransfer::dtSend(const PeerId &peer,
                            const DataTransferMessage &msg) const {
    dtSend({peer, {}}, msg);
  }

  void DataTransfer::dtSend(const PeerInfo &peer,
                            const DataTransferMessage &msg) const {
    streams->open({
        peer,
        kProtocol,
        [msg](Host::StreamResult _stream) {
          if (_stream) {
            auto stream{std::make_shared<CborStream>(_stream.value())};
            stream->write(msg, [stream](outcome::result<size_t>) {});
          }
        },
    });
  }
}  // namespace fc::data_transfer
