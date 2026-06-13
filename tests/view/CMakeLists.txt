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
        adaptor_test
        addressof_test
        adjacent_remove_if_test
        all_test
        any_view_test
        cache1_test
        cartesian_product_test
        chunk_by_test
        chunk_test
        common_test
        concat_test
        const_test
        conversion_test
        counted_test
        cycle_test
        delimit_test
        drop_exactly_test
        drop_last_test
        drop_test
        drop_while_test
        enumerate_test
        exclusive_scan_test
        facade_test
        generate_n_test
        generate_test
        getlines_test
        group_by_test
        indirect_test
        intersperse_test
        iota_test
        istream_test
        iterator_range_test
        join_test
        keys_value_test
        linear_distribute_test
        move_test
        partial_sum_depr_test
        partial_sum_test
        remove_if_test
        remove_test
        repeat_test
        replace_if_test
        replace_test
        reverse_test
        sample_test
        set_difference_test
        set_intersection_test
        set_symmetric_difference_test
        set_union_test
        slice_test
        sliding_test
        span_test
        split_test
        stride_test
        subrange_test
        tail_test
        take_exactly_test
        take_last_test
        take_test
        take_while_test
        tokenize_test
        transform_test
        trim_test
        unique_test
        view_test
        zip_test
)

foreach (case IN LISTS AL_CASE)
    kmcmake_cc_test(
            NAME ${case}
            MODULE view
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
