# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)
include(${PARENT_SRC_DIR}/engines/pmemobj_based/helpers.cmake)

setup()

pmempool_execute(create -l ${LAYOUT} -s 100M obj ${DIR}/testfile)
execute(${TEST_EXECUTABLE} ${ENGINE} ${DIR}/testfile)

finish()
