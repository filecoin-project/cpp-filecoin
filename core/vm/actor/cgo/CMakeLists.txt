#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_custom_target(go_actors_build
    COMMAND ./go_actors.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/go_actors.a
    )

add_library(go_actors SHARED IMPORTED GLOBAL)
set_target_properties(go_actors PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/go_actors.a
    )
add_dependencies(go_actors
    go_actors_build
    )
if (APPLE)
    target_link_libraries(go_actors INTERFACE "-framework CoreFoundation -framework Security")
endif ()

add_library(cgo_actors
    actors.cpp
    )
target_link_libraries(cgo_actors
    go_actors
    runtime
    zerocomm
    )
