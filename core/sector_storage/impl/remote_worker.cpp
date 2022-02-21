/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/beast/http.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "primitives/jwt/jwt.hpp"

#include "common/uri_parser/uri_parser.hpp"
#include "drand/impl/http.cpp"
#include "primitives/piece/piece_data.hpp"
#include "sector_storage/impl/remote_worker.hpp"

namespace fc::sector_storage {
  using drand::http::ClientSession;
  using primitives::piece::MetaPieceData;
  using primitives::piece::ReaderType;
  namespace beast = boost::beast;
  namespace net = boost::asio;
  using tcp = net::ip::tcp;
  namespace uuids = boost::uuids;
  namespace http = boost::beast::http;
  using common::HttpUri;
  using primitives::jwt::kAdminPermission;

  outcome::result<std::shared_ptr<RemoteWorker>>
  RemoteWorker::connectRemoteWorker(io_context &context,
                                    const std::shared_ptr<CommonApi> &api,
                                    const std::string &address) {
    OUTCOME_TRY(uri, HttpUri::parse(address));

    OUTCOME_TRY(token, api->AuthNew({kAdminPermission}));

    struct make_unique_enabler : public RemoteWorker {
      explicit make_unique_enabler(io_context &context)
          : RemoteWorker{context} {};
    };

    std::unique_ptr<RemoteWorker> r_worker =
        std::make_unique<make_unique_enabler>(context);

    r_worker->port_ = std::to_string(uri.port());
    r_worker->host_ = uri.host();
    r_worker->wsc_.setup(r_worker->api_);

    OUTCOME_TRY(
        r_worker->wsc_.connect(uri.host(),
                               std::to_string(uri.port()),
                               "/rpc/v0",
                               std::string{token.begin(), token.end()}));

    return std::move(r_worker);
  }

  outcome::result<primitives::WorkerInfo> RemoteWorker::getInfo() {
    return api_.Info();
  }

  outcome::result<std::set<primitives::TaskType>>
  RemoteWorker::getSupportedTask() {
    return api_.TaskTypes();
  }

  outcome::result<std::vector<primitives::StoragePath>>
  RemoteWorker::getAccessiblePaths() {
    return api_.Paths();
  }

  RemoteWorker::RemoteWorker(io_context &context)
      : wsc_(*(worker_thread_.io)), io_(context) {}

  struct PieceDataSender {
    explicit PieceDataSender(io_context &io)
        : resolver_{io}, stream_{net::make_strand(io)} {}

    PieceDataSender(const PieceDataSender &) = delete;
    PieceDataSender(PieceDataSender &&) = delete;
    ~PieceDataSender() {
      boost::system::error_code ec;
      stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    PieceDataSender &operator=(const PieceDataSender &) = delete;
    PieceDataSender &operator=(PieceDataSender &&) = delete;

    static void send(
        int fd,
        io_context &io,
        const std::string &host,
        const std::string &port,
        const std::string &target,
        const std::uint64_t piece_size,
        const std::function<void(outcome::result<std::string>)> &cb) {
      auto sender{std::make_shared<PieceDataSender>(io)};
      boost::system::error_code error;
      boost::beast::file_posix fp;
      fp.native_handle(fd);

      sender->file_req_.body().reset(std::move(fp), error);
      sender->file_req_.method(http::verb::post);
      sender->file_req_.target(target);
      sender->file_req_.set(http::field::host, host);
      sender->file_req_.set(http::field::content_length, piece_size);
      sender->resolver_.async_resolve(
          host, port, [sender, MOVE(cb)](auto &&ec, auto &&iterator) {
            EC_CB();
            sender->stream_.async_connect(
                iterator, [sender, MOVE(cb)](auto &&ec, auto &&) {
                  EC_CB();
                  http::async_write(
                      sender->stream_,
                      sender->file_req_,
                      [sender, MOVE(cb)](auto &&ec, auto &&) {
                        EC_CB();
                        http::async_read(
                            sender->stream_,
                            sender->buffer_,
                            sender->res_,
                            [sender, MOVE(cb)](auto &&ec, auto &&) {
                              EC_CB();
                              cb(std::move(sender->res_.body()));
                            });
                      });
                });
          });
    }

   private:
    http::request<http::file_body> file_req_;
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::response<http::string_body> res_;
  };

  outcome::result<CallId> RemoteWorker::addPiece(
      const SectorRef &sector,
      VectorCoW<UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      PieceData piece_data) {
    MetaPieceData meta_data =
        piece_data.isNullData()
            ? MetaPieceData(std::to_string(new_piece_size),
                            ReaderType::Type::kNullReader)
            : MetaPieceData(uuids::to_string(uuids::random_generator()()),
                            ReaderType::Type::kPushStreamReader);
    if (!piece_data.isNullData()) {
      PieceDataSender::send(
          piece_data.release(),
          io_,
          host_,
          port_,
          "/rpc/streams/v0/push/" + meta_data.info,
          new_piece_size,
          [](const outcome::result<std::string> &res) {
            if (res.has_error()) {
              spdlog::error("Transfer of pieces was finished with error: {}",
                            res.value());
            } else {
              spdlog::info("Transfer of pieces was finished with response {}",
                           res.value());
            }
          });
    }
    return api_.AddPiece(sector, piece_sizes.into(), new_piece_size, meta_data);
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const std::vector<PieceInfo> &pieces) {
    return api_.SealPreCommit1(sector, ticket, pieces);
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit2(
      const SectorRef &sector, const PreCommit1Output &pre_commit_1_output) {
    return api_.SealPreCommit2(sector, pre_commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      const std::vector<PieceInfo> &pieces,
      const SectorCids &cids) {
    return api_.SealCommit1(sector, ticket, seed, pieces, cids);
  }

  outcome::result<CallId> RemoteWorker::sealCommit2(
      const SectorRef &sector, const Commit1Output &commit_1_output) {
    return api_.SealCommit2(sector, commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::finalizeSector(
      const SectorRef &sector, std::vector<Range> keep_unsealed) {
    return api_.FinalizeSector(sector, keep_unsealed);
  }

  outcome::result<CallId> RemoteWorker::replicaUpdate(
      const SectorRef &sector, const std::vector<PieceInfo> &pieces) {
    return api_.ReplicaUpdate(sector, pieces);
  }

  outcome::result<CallId> RemoteWorker::proveReplicaUpdate1(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed) {
    return api_.ProveReplicaUpdate1(
        sector, sector_key, new_sealed, new_unsealed);
  }

  outcome::result<CallId> RemoteWorker::proveReplicaUpdate2(
      const SectorRef &sector,
      const CID &sector_key,
      const CID &new_sealed,
      const CID &new_unsealed,
      const Update1Output &update_1_output) {
    return api_.ProveReplicaUpdate2(
        sector, sector_key, new_sealed, new_unsealed, update_1_output);
  }

  outcome::result<CallId> RemoteWorker::moveStorage(const SectorRef &sector,
                                                    SectorFileType types) {
    return api_.MoveStorage(sector, types);
  }

  outcome::result<CallId> RemoteWorker::unsealPiece(
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &unsealed_cid) {
    return api_.UnsealPiece(sector, offset, size, randomness, unsealed_cid);
  }

  outcome::result<CallId> RemoteWorker::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size) {
    return WorkerErrors::kUnsupportedCall;
  }

  outcome::result<CallId> RemoteWorker::fetch(const SectorRef &sector,
                                              const SectorFileType &file_type,
                                              PathType path_type,
                                              AcquireMode mode) {
    return api_.Fetch(sector, file_type, path_type, mode);
  }

  void RemoteWorker::ping(std::function<void(const bool resp)> cb) {
    api_.Version([=](auto res) { cb(!res.has_error()); });
  }
}  // namespace fc::sector_storage
