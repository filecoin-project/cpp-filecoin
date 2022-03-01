#.rst:
# Findleveldb.cmake
# -----------
# Finds the leveldb library
#
# This will define the following variables:
#
#   LEVELDB_FOUND        -  system has leveldb
#   LEVELDB_INCLUDE_DIRS -  leveldb include directories
#   LEVELDB_LIBRARIES    -  libraries needed to use leveldb
#
# and the following imported targets:
#
#   leveldb::leveldb   - The leveldb library
#

include(FindPackageHandleStandardArgs)

find_path(
  LEVELDB_INCLUDE_DIR
  NAMES leveldb/db.h
  HINTS "${LEVELDB_ROOT_DIR}" "$ENV{LEVELDB_ROOT_DIR}"
  PATH_SUFFIXES include
)

find_library(
  LEVELDB_LIBRARY
  NAMES leveldb
  HINTS "${LEVELDB_ROOT_DIR}" "$ENV{LEVELDB_ROOT_DIR}"
  PATH_SUFFIXES "${LIBRARY_PATH_PREFIX}"
)

find_package_handle_standard_args(
  leveldb
  DEFAULT_MSG
    LEVELDB_INCLUDE_DIR
    LEVELDB_LIBRARY
)

if (LEVELDB_FOUND)
  set(LEVELDB_INCLUDE_DIRS "${LEVELDB_INCLUDE_DIR}")
  set(LEVELDB_LIBRARIES "${LEVELDB_LIBRARY}")

  if (NOT TARGET leveldb::leveldb)
    add_library(leveldb::leveldb UNKNOWN IMPORTED)
    set_target_properties(leveldb::leveldb PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LEVELDB_INCLUDE_DIR}"
      IMPORTED_LOCATION "${LEVELDB_LIBRARY}"
    )
  endif()
endif()

mark_as_advanced(LEVELDB_INCLUDE_DIR)
mark_as_advanced(LEVELDB_LIBRARY)
