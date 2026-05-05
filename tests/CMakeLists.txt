# Copyright (C) Kumo inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#[[kmcmake_cc_test(
        NAME custom_comparator_test
        SOURCES custom_comparator_test.cc
        DEPS turbo::turbo ${KMCMAKE_DEPS_LINK} ${GTEST_LIB} ${GTEST_MAIN_LIB}
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)]]
#[[
kmcmake_cc_test(
        NAME foo_test
        SOURCES foo_test.cc
        DEPS ${PROJECT}::foo ${KMCMAKE_DEPS_LINK} gtest gtest_main
        COPTS ${KMCMAKE_CXX_OPTIONS}
)

kmcmake_cc_test(
        NAME foo_doctest
        SOURCES foo_doctest.cc
        DEPS ${PROJECT}::foo ${KMCMAKE_DEPS_LINK} doctest
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)
]]

configure_file(${PROJECT_SOURCE_DIR}/tests/config.h.in ${PROJECT_SOURCE_DIR}/tests/config.h @ONLY)

kmcmake_cc_test(
        NAME raw_test
        MODULE base
        SOURCES raw_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)

kmcmake_cc_test(
        NAME pass_test
        MODULE base
        SOURCES pass_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
        EXT
)

# base_pass_test_1
kmcmake_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS 1
        PASS_EXP "pass;Passed"
)

# base_pass_test_2
kmcmake_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS 2
        PASS_EXP "pass;Passed"
)
# base_pass_test_foo
kmcmake_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS foo
        PASS_EXP "pass;Passed"
)

# base_pass_test_bar not run
kmcmake_cc_test_ext(
        NAME pass_test
        MODULE base
        ALIAS bar
        PASS_EXP "pass;Passed"
        DISABLED
)

kmcmake_cc_test(
        NAME args_test
        MODULE base
        SOURCES args_test.cc
        EXT
)

# base_args_test_fail fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS fail
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
)

# base_args_test_fail fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS fail_args
        ARGS "Failed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
)

# base_args_test_skip fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_fail
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip
        ARGS "SKIP"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_pass
        ARGS "Passed"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
)

# base_args_test_skip fail
kmcmake_cc_test_ext(
        NAME args_test
        MODULE base
        ALIAS skip_diabled
        ARGS "Passed"
        PASS_EXP "pass;Passed"
        FAIL_EXP "[^a-z]Error;ERROR;Failed"
        SKIP_EXP "[^a-z]Skip" "SKIP" "Skipped"
        DISABLED
)

kmcmake_cc_test(
        NAME raw_test
        MODULE norun
        SOURCES raw_test.cc
        CXXOPTS ${KMCMAKE_CXX_OPTIONS}
)