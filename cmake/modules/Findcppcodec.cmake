#.rst:
# Findcppcodec.cmake
# ------------------
# Try to find cppcodec
#
# This will define the following variables:
#
# CPPCODEC_FOUND - system has cppcodec
# CPPCODEC_INCLUDE_DIRS - cppcodec include directories
#

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CPPCODEC cppcodec)
endif()

if (PC_CPPCODEC_FOUND)
  set(CPPCODEC_FOUND "${PC_CPPCODEC_FOUND}")
  set(CPPCODEC_INCLUDE_DIRS "${PC_CPPCODEC_INCLUDEDIR}")
endif()
