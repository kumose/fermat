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
        adjacent_find_test
        adjacent_remove_if_test
        all_of_test
        any_of_test
        binary_search_test
        contains_test
        contains_subrange_test
        copy_backward_test
        copy_if_test
        copy_n_test
        copy_test
        count_test
        count_if_test
        ends_with_test
        equal_range_test
        equal_test
        fill_test
        fill_n_test
        find_test
        find_end_test
        find_first_of_test
        find_if_test
        find_if_not_test
        fold_test
        for_each_test
        for_each_n_test
        generate_test
        generate_n_test
        includes_test
        inplace_merge_test
        is_heap1_test
        is_heap2_test
        is_heap3_test
        is_heap4_test
        is_heap_until1_test
        is_heap_until2_test
        is_heap_until3_test
        is_heap_until4_test
        is_partitioned_test
        is_permutation_test
        is_sorted_test
        is_sorted_until_test
        lexicographical_compare_test
        lower_bound_test
        make_heap_test
        max_test
        max_element_test
        merge_test
        min_test
        min_element_test
        minmax_test
        minmax_element_test
        move_backward_test
        move_test
        next_permutation_test
        none_of_test
        nth_element_test
        partial_sort_test
        partial_sort_copy_test
        partition_test
        partition_copy_test
        #partition_move_test
        partition_point_test
        prev_permutation_test
        push_heap_test
        remove_test
        remove_if_test
        remove_copy_if_test
        remove_copy_test
        replace_copy_if_test
        replace_copy_test
        replace_if_test
        replace_test
        reverse_test
        reverse_copy_test
        rotate_test
        rotate_copy_test
        sample_test
        search_test
        search_n_test
        set_difference1_test
        set_difference2_test
        set_difference3_test
        set_difference4_test
        set_difference5_test
        set_difference6_test
        set_intersection1_test
        set_intersection2_test
        set_intersection3_test
        set_intersection4_test
        set_intersection5_test
        set_intersection6_test
        set_symmetric_difference1_test
        set_symmetric_difference2_test
        set_symmetric_difference3_test
        set_symmetric_difference4_test
        set_symmetric_difference5_test
        set_symmetric_difference6_test
        shuffle_test
        sort_test
        stable_partition_test
        stable_sort_test
        starts_with_test
        swap_ranges_test
        transform_test
        unique_test
        unique_copy_test
        unstable_remove_if_test
        upper_bound_test
)

foreach (case IN LISTS AL_CASE)
    kmcmake_cc_test(
            NAME ${case}
            MODULE algo
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
