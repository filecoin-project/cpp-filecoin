/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/tarutil.hpp"

#include <libarchive/archive.h>
#include <libarchive/archive_entry.h>
#include <boost/filesystem.hpp>
#include "common/ffi.hpp"
#include "common/logger.hpp"

namespace fs = boost::filesystem;

namespace fc::common {

  static Logger logger = createLogger("tar util");

  int copy_data(struct archive *ar, struct archive *aw) {
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
      r = archive_read_data_block(ar, &buff, &size, &offset);
      if (r == ARCHIVE_EOF) {
        return (ARCHIVE_OK);
      }
      if (r < ARCHIVE_OK) {
        return (r);
      }
      r = archive_write_data_block(aw, buff, size, offset);
      if (r < ARCHIVE_OK) {
        return (r);
      }
    }
  }

  outcome::result<void> extractTar(const std::string &tar_path,
                                   const std::string &output_path) {
    if (!fs::exists(output_path)) {
      boost::system::error_code ec;
      if (!fs::create_directories(output_path, ec)) {
        if (ec.failed()) {
          logger->error("Extract tar: {}", ec.message());
        }
        return TarErrors::kCannotCreateDir;
      }
    }

    struct archive_entry *entry;
    int flags;
    int r;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    auto a = ffi::wrap(archive_read_new(), archive_read_free);
    archive_read_support_format_tar(a.get());
    auto ext = ffi::wrap(archive_write_disk_new(), archive_write_free);
    archive_write_disk_set_options(ext.get(), flags);
    archive_write_disk_set_standard_lookup(ext.get());
    if (archive_read_open_filename(a.get(), tar_path.c_str(), kTarBlockSize)
        != ARCHIVE_OK) {
      logger->error("Extract tar: {}", archive_error_string(a.get()));
      return TarErrors::kCannotUntarArchive;
    }
    for (;;) {
      r = archive_read_next_header(a.get(), &entry);
      if (r == ARCHIVE_EOF) {
        break;
      }
      if (r < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(a.get()));
        return TarErrors::kCannotUntarArchive;
      }
      if (r < ARCHIVE_OK) {
        logger->warn("Extract tar: {}", archive_error_string(a.get()));
      }

      std::string currentFile(archive_entry_pathname(entry));
      archive_entry_set_pathname(entry,
                                 (fs::path(output_path) / currentFile).c_str());
      r = archive_write_header(ext.get(), entry);
      if (r < ARCHIVE_OK) {
        if (r < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(a.get()));
        } else {
          logger->warn("Extract tar: {}", archive_error_string(a.get()));
        }
      } else if (archive_entry_size(entry) > 0) {
        r = copy_data(a.get(), ext.get());
        if (r < ARCHIVE_WARN) {
          logger->error("Extract tar: {}", archive_error_string(a.get()));
          return TarErrors::kCannotUntarArchive;
        }
        if (r < ARCHIVE_OK) {
          logger->warn("Extract tar: {}", archive_error_string(a.get()));
        }
      }
      r = archive_write_finish_entry(ext.get());
      if (r < ARCHIVE_WARN) {
        logger->error("Extract tar: {}", archive_error_string(a.get()));
        return TarErrors::kCannotUntarArchive;
      }
      if (r < ARCHIVE_OK) {
        logger->warn("Extract tar: {}", archive_error_string(a.get()));
      }
    }
    archive_read_close(a.get());
    archive_write_close(ext.get());

    return outcome::success();
  }

}  // namespace fc::common

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, TarErrors, e) {
  using fc::common::TarErrors;
  switch (e) {
    case (TarErrors::kCannotCreateDir):
      return "Tar Util: cannot create output dir";
    case (TarErrors::kCannotUntarArchive):
      return "Tar Util: cannot untar archive";
    default:
      return "Tar Util: unknown error";
  }
}
