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
        NAME string_test
        MODULE con
        SOURCES
        string_test.cc
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
        NAME vector_test
        MODULE con
        SOURCES
        vector_test.cc
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
        NAME vector_map_test
        MODULE con
        SOURCES
        vector_map_test.cc
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
        NAME vector_set_test
        MODULE con
        SOURCES
        vector_set_test.cc
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
        NAME enumerator_test
        MODULE con
        SOURCES
        enumerator_test.cc
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
        MODULE con
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
        NAME intrusive_list_test
        MODULE con
        SOURCES
        intrusive_list_test.cc
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
        NAME cord_buffer_test
        MODULE con
        SOURCES
        cord_buffer_test.cc
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
        NAME cord_buffer_stream_test
        MODULE con
        SOURCES
        cord_buffer_stream_test.cc
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
        NAME peeker_test
        MODULE con
        SOURCES
        peeker_test.cc
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
        NAME customer_test
        MODULE con
        SOURCES
        customer_test.cc
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

