/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_ALGORITHM_HPP
#define RANGES_V3_ALGORITHM_HPP

#include <fermat/algorithm/adjacent_find.h>
#include <fermat/algorithm/adjacent_remove_if.h>
#include <fermat/algorithm/all_of.h>
#include <fermat/algorithm/any_of.h>
#include <fermat/algorithm/binary_search.h>
#include <fermat/algorithm/contains.h>
#include <fermat/algorithm/contains_subrange.h>
#include <fermat/algorithm/copy.h>
#include <fermat/algorithm/copy_backward.h>
#include <fermat/algorithm/copy_if.h>
#include <fermat/algorithm/copy_n.h>
#include <fermat/algorithm/count.h>
#include <fermat/algorithm/count_if.h>
#include <fermat/algorithm/ends_with.h>
#include <fermat/algorithm/equal.h>
#include <fermat/algorithm/equal_range.h>
#include <fermat/algorithm/fill.h>
#include <fermat/algorithm/fill_n.h>
#include <fermat/algorithm/find.h>
#include <fermat/algorithm/find_end.h>
#include <fermat/algorithm/find_first_of.h>
#include <fermat/algorithm/find_if.h>
#include <fermat/algorithm/find_if_not.h>
#include <fermat/algorithm/fold.h>
#include <fermat/algorithm/for_each.h>
#include <fermat/algorithm/for_each_n.h>
#include <fermat/algorithm/generate.h>
#include <fermat/algorithm/generate_n.h>
#include <fermat/algorithm/heap_algorithm.h>
#include <fermat/algorithm/inplace_merge.h>
#include <fermat/algorithm/is_partitioned_test.h>
#include <fermat/algorithm/is_sorted.h>
#include <fermat/algorithm/is_sorted_until.h>
#include <fermat/algorithm/lexicographical_compare.h>
#include <fermat/algorithm/lower_bound.h>
#include <fermat/algorithm/max.h>
#include <fermat/algorithm/max_element.h>
#include <fermat/algorithm/merge.h>
#include <fermat/algorithm/min.h>
#include <fermat/algorithm/min_element_test.h>
#include <fermat/algorithm/minmax.h>
#include <fermat/algorithm/minmax_element.h>
#include <fermat/algorithm/mismatch.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/move_backward.h>
#include <fermat/algorithm/none_of.h>
#include <fermat/algorithm/nth_element.h>
#include <fermat/algorithm/partial_sort.h>
#include <fermat/algorithm/partial_sort_copy.h>
#include <fermat/algorithm/partition.h>
#include <fermat/algorithm/partition_copy.h>
#include <fermat/algorithm/partition_point.h>
#include <fermat/algorithm/permutation.h>
#include <fermat/algorithm/remove_test.h>
#include <fermat/algorithm/remove_copy.h>
#include <fermat/algorithm/remove_copy_if.h>
#include <fermat/algorithm/remove_if.h>
#include <fermat/algorithm/replace.h>
#include <fermat/algorithm/replace_copy.h>
#include <fermat/algorithm/replace_copy_if.h>
#include <fermat/algorithm/replace_if.h>
#include <fermat/algorithm/reverse.h>
#include <fermat/algorithm/reverse_copy.h>
#include <fermat/algorithm/rotate.h>
#include <fermat/algorithm/rotate_copy.h>
#include <fermat/algorithm/sample.h>
#include <fermat/algorithm/search.h>
#include <fermat/algorithm/search_n.h>
#include <fermat/algorithm/set_algorithm.h>
#include <fermat/algorithm/shuffle.h>
#include <fermat/algorithm/sort.h>
#include <fermat/algorithm/stable_partition.h>
#include <fermat/algorithm/stable_sort.h>
#include <fermat/algorithm/starts_with.h>
#include <fermat/algorithm/swap_ranges.h>
#include <fermat/algorithm/transform.h>
#include <fermat/algorithm/unique.h>
#include <fermat/algorithm/unique_copy.h>
#include <fermat/algorithm/unstable_remove_if.h>
#include <fermat/algorithm/upper_bound.h>
#include <fermat/detail/config.h>

// BUGBUG
#include <fermat/algorithm/aux_/equal_range_n.h>
#include <fermat/algorithm/aux_/lower_bound_n.h>
#include <fermat/algorithm/aux_/merge_n.h>
#include <fermat/algorithm/aux_/merge_n_with_buffer.h>
#include <fermat/algorithm/aux_/sort_n_with_buffer.h>
#include <fermat/algorithm/aux_/upper_bound_n.h>

#endif
