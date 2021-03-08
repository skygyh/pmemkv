# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020, Intel Corporation

include(${PARENT_SRC_DIR}/helpers.cmake)

setup()

make_config({"path":"${DIR}/testfile","force_create":1,"size":838860800}) #80MB
execute(${TEST_EXECUTABLE} ${ENGINE} ${CONFIG} ${PARAMS})

finish()
