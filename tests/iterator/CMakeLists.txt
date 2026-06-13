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

set(AL_CASE
        basic_iterator_test
        common_iterator_test
        counted_iterator_test
        diffmax_test
        iterator_test
        reverse_iterator_test
        unformatted_ostream_iterator_test
)

foreach (case IN LISTS AL_CASE)
    kmcmake_cc_test(
            NAME ${case}
            MODULE range_itr
            SOURCES
            ${case}.cc
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
endforeach ()
