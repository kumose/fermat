/// @file allocator_test.cpp
/// @brief Unit tests for fermat::BasicAllocator and related memory utilities.

#include <gtest/gtest.h>
#include <fermat/memory/allocator.h>
#include <vector>
#include <atomic>
#include <new>

namespace fermat {
    /// Helper type to track construction and destruction counts.
    struct Tracked {
        static std::atomic<int> constructed; ///< Total constructed objects.
        static std::atomic<int> destructed; ///< Total destructed objects.
        int value; ///< User value.

        Tracked(int v = 0) : value(v) { ++constructed; }

        Tracked(const Tracked &) = delete;

        Tracked(Tracked &&) = delete;

        ~Tracked() { ++destructed; }
    };

    std::atomic<int> Tracked::constructed{0};
    std::atomic<int> Tracked::destructed{0};

    // ============================================================================
    /// Tests for BasicAllocator<T, Alignment, Operator>
    // ============================================================================

    TEST(BasicAllocatorTest, AllocateDeallocate) {
        /// Allocator for int with default alignment (0 = automatic).
        using Alloc = BasicAllocator<int, 0>;
        Alloc alloc;
        size_t n = 10; ///< Request 10 elements.
        int *p = alloc.allocate(&n); ///< n may be rounded up by the pool.
        ASSERT_NE(p, nullptr);

        /// Write and verify data across the allocated block.
        for (size_t i = 0; i < n; ++i) p[i] = static_cast<int>(i);
        for (size_t i = 0; i < n; ++i)
            EXPECT_EQ(p[i], static_cast<int>(i));

        alloc.deallocate(p, n); ///< Return memory using the actual size.
    }

    TEST(BasicAllocatorTest, ConstructDestroy) {
        using Alloc = BasicAllocator<Tracked, 0>;
        Alloc alloc;
        Tracked::constructed = 0;
        Tracked::destructed = 0;

        size_t n = 1;
        Tracked *p = alloc.allocate(&n);
        ASSERT_NE(p, nullptr);

        /// Placement new to construct the object.
        ::new(p) Tracked(42);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ(Tracked::constructed.load(), 1);

        /// Explicit destructor call before deallocation.
        p->~Tracked();
        EXPECT_EQ(Tracked::destructed.load(), 1);
        alloc.deallocate(p, n);
    }

    TEST(BasicAllocatorTest, GoodSize) {
        using Alloc = BasicAllocator<int, 0>;
        Alloc alloc;

        /// good_size() returns the minimal allocation size that satisfies the request.
        EXPECT_GE(alloc.good_size(10), 10);
        /// For small sizes, the tiered allocator rounds up to kTinySize (64).
        EXPECT_EQ(alloc.good_size(10), 64);
    }

    TEST(BasicAllocatorTest, Equality) {
        using Alloc = BasicAllocator<int, 0>;
        Alloc a1, a2;
        /// Stateless allocators are always equal.
        EXPECT_TRUE(a1 == a2);
        EXPECT_FALSE(a1 != a2);
    }

    TEST(BasicAllocatorTest, CollectAndApplyArena) {
        using Alloc = BasicAllocator<int, 0>;
        Alloc alloc;
        const int N = 10; ///< Number of objects to allocate.
        size_t elem_size = 1; ///< Request one element per allocation.
        std::vector<int *> ptrs;

        {
            auto cleanup = Alloc::collect_arena();
        }
        /// 1. Allocate and then deallocate – objects go into thread‑local free list.
        for (int i = 0; i < N; ++i) {
            size_t n = elem_size;
            int *p = alloc.allocate(&n);
            KLOG(INFO) << "alloc:" << n;
            ptrs.push_back(p);
        }
        for (int *p: ptrs) {
            alloc.deallocate(p, elem_size);
        }

        KLOG(INFO) << "**********************************";
        /// 2. Collect all free objects from the thread‑local cache as ObjectGuards.
        auto guards = Alloc::collect_arena(); ///< Returns vector<ObjectGuard<0>>.
        size_t total = 0;
        for (auto &g: guards) {
            total += g.ptrs.size();
            KLOG(INFO) << g.n / 8 << " size:" << g.ptrs.size();
        }
        EXPECT_EQ(total, N);

        /// 3. Return the collected memory back to the thread‑local cache.
        Alloc::apply_arena(guards);

        /// 4. Clean up: collect again and let the guards destroy the memory.
        {
            auto cleanup = Alloc::collect_arena();
            total = 0;
            for (auto &g: cleanup) {
                total += g.ptrs.size();
                KLOG(INFO) << g.n / 8 << " size:" << g.ptrs.size();
            }
            EXPECT_EQ(total, N);
        }

        {
            auto cleanup = Alloc::collect_arena();
            total = 0;
            for (auto &g: cleanup) {
                total += g.ptrs.size();
            }
            EXPECT_EQ(total, 0);
        }

        KLOG(INFO) << "**********************************";
        KLOG(INFO) << "collected:" << 1;
    }

    TEST(BasicAllocatorTest, VariousSizes) {
        using Alloc = BasicAllocator<int, 0>;
        Alloc alloc;

        // Purge previous TLS state
        {
            auto cleanup = Alloc::collect_arena();
        }

        std::vector<size_t> requests = {
            1, 64, 65, 256, 257, 512, 513, 1024, 1025, 4096, 4097
        };

        struct Record {
            int *ptr;
            size_t requested;
            size_t actual; // actual number of elements allocated (after rounding)
        };
        std::vector<Record> records;

        // 1. Allocate
        for (size_t req: requests) {
            size_t n = req; // allocate expects a pointer, but won't be modified
            int *p = alloc.allocate(&n);
            // Get the actual element count using good_size (rounded up to pool size)
            size_t actual = alloc.good_size(req);
            records.push_back({p, req, actual});
        }

        // 2. Deallocate using the actual size
        for (auto &rec: records) {
            alloc.deallocate(rec.ptr, rec.actual);
        }

        // 3. Collect all free objects from TLS
        auto guards = Alloc::collect_arena();

        // 4. Verify that each pointer belongs to the correct pool guard
        for (auto &rec: records) {
            size_t actual_elements = rec.actual;
            size_t expected_block_bytes = 0;

            // Map actual_elements to the expected block size (bytes)
            if (actual_elements == 1) {
                expected_block_bytes = AlignedBytesAllocator<int, 1, 0>::good_size();
            } else if (actual_elements <= kTinySize) {
                EXPECT_EQ(actual_elements, kTinySize);
                expected_block_bytes = AlignedBytesAllocator<int, kTinySize, 0>::good_size();
            } else if (actual_elements <= kMiniSize) {
                EXPECT_EQ(actual_elements, kMiniSize);
                expected_block_bytes = AlignedBytesAllocator<int, kMiniSize, 0>::good_size();
            } else if (actual_elements <= kSmallSize) {
                EXPECT_EQ(actual_elements, kSmallSize);
                expected_block_bytes = AlignedBytesAllocator<int, kSmallSize, 0>::good_size();
            } else if (actual_elements <= kMediumSize) {
                EXPECT_EQ(actual_elements, kMediumSize);
                expected_block_bytes = AlignedBytesAllocator<int, kMediumSize, 0>::good_size();
            } else if (actual_elements <= kBigSize) {
                EXPECT_EQ(actual_elements, kBigSize);
                expected_block_bytes = AlignedBytesAllocator<int, kBigSize, 0>::good_size();
            } else if (actual_elements <= kPageSize) {
                EXPECT_EQ(actual_elements, kPageSize);
                expected_block_bytes = AlignedBytesAllocator<int, kPageSize, 0>::good_size();
            } else {
                // Generic allocation (no pooling)
                EXPECT_GE(actual_elements, rec.requested);
                continue; // not tracked in guards
            }

            bool found = false;
            for (auto &g: guards) {
                if (g.n == expected_block_bytes) {
                    for (void *ptr: g.ptrs) {
                        if (ptr == rec.ptr) {
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
            }
            EXPECT_TRUE(found) << "Missing pointer for requested size " << rec.requested
                           << " (actual elements " << actual_elements
                           << ", block bytes " << expected_block_bytes << ")";
        }

        // 5. Return guards to TLS and then collect + destroy to free memory
        Alloc::apply_arena(guards);
        {
            auto cleanup = Alloc::collect_arena();
        } // all blocks freed

        // 6. Final check: TLS should be empty
        auto leftover = Alloc::collect_arena();
        size_t total = 0;
        for (auto &g: leftover) total += g.ptrs.size();
        EXPECT_EQ(total, 0);
    }
} // namespace fermat
