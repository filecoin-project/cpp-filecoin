# Try to find cppcodec
# Once done, this will define
#
# CPPCODEC_FOUND        -  system has cppcodec

find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(CPPCODEC cppcodec)
endif()
