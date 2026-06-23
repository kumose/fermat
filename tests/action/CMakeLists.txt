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

kmcmake_cc_test(
        NAME adjacent_remove_if_test
        MODULE action
        SOURCES
        adjacent_remove_if_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME cont_concepts_test
        MODULE range_action
        SOURCES
        cont_concepts_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME drop_test
        MODULE range_action
        SOURCES
        drop_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME drop_while_test
        MODULE range_action
        SOURCES
        drop_while_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME insert_test
        MODULE range_action
        SOURCES
        insert_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME join_test
        MODULE range_action
        SOURCES
        join_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME push_back_test
        MODULE range_action
        SOURCES
        push_back_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME push_front_test
        MODULE range_action
        SOURCES
        push_front_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME remove_test
        MODULE range_action
        SOURCES
        remove_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME remove_if_test
        MODULE range_action
        SOURCES
        remove_if_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME reverse_test
        MODULE range_action
        SOURCES
        reverse_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME shuffle_test
        MODULE range_action
        SOURCES
        shuffle_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME slice_test
        MODULE range_action
        SOURCES
        slice_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME sort_test
        MODULE range_action
        SOURCES
        sort_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME split_test
        MODULE range_action
        SOURCES
        split_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME stable_sort_test
        MODULE range_action
        SOURCES
        stable_sort_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME stride_test
        MODULE range_action
        SOURCES
        stride_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)

kmcmake_cc_test(
        NAME take_test
        MODULE range_action
        SOURCES
        take_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME take_while_test
        MODULE range_action
        SOURCES
        take_while_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME transform_test
        MODULE range_action
        SOURCES
        transform_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME unique_test
        MODULE range_action
        SOURCES
        unique_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)


kmcmake_cc_test(
        NAME unstable_remove_if_test
        MODULE range_action
        SOURCES
        unstable_remove_if_test.cc
        CXXOPTS
        ${KMCMAKE_CXX_OPTIONS}
        -fno-access-control
        -Wno-unused-result
        LINKS
        fermat::fermat_static
        GTest::gtest
        GTest::gtest_main
        ${KMCMAKE_DEPS_LINK}
)
