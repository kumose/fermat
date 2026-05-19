// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// iobuf - A non-continuous zero-copied buffer

// Date: Thu Nov 22 13:57:56 CST 2012

#include "butil/ssl_compat.h"               // BIO_fd_non_fatal_error
#include <openssl/err.h>
#include <openssl/ssl.h>                   // SSL_*
#ifdef USE_MESALINK
#include <mesalink/openssl/ssl.h>
#include <mesalink/openssl/err.h>
#endif
#include <sys/syscall.h>                   // syscall
#include <fcntl.h>                         // O_RDONLY
#include <cerrno>                         // errno
#include <climits>                        // CHAR_BIT
#include <stdexcept>                       // std::invalid_argument
#include <fermat/io/cord_buffer.h>
#include <fermat/io/fd_guard.h>
#include <turbo/threading/platform_thread.h>
#include <turbo/flags/flag.h>
#include <fermat/memory/allocator.h>

TURBO_FLAG(int32_t, iobuf_aligned_buf_block_size, 0, "cord buffer aligned buf block size");

namespace fermat {
    namespace iobuf {
        typedef ssize_t (*iov_function)(int fd, const struct iovec *vector,
                                        int count, off_t offset);

        // Userpsace preadv
        static ssize_t user_preadv(int fd, const struct iovec *vector,
                                   int count, off_t offset) {
            ssize_t total_read = 0;
            for (int i = 0; i < count; ++i) {
                const ssize_t rc = ::pread(fd, vector[i].iov_base, vector[i].iov_len, offset);
                if (rc <= 0) {
                    return total_read > 0 ? total_read : rc;
                }
                total_read += rc;
                offset += rc;
                if (rc < (ssize_t) vector[i].iov_len) {
                    break;
                }
            }
            return total_read;
        }

        static ssize_t user_pwritev(int fd, const struct iovec *vector,
                                    int count, off_t offset) {
            ssize_t total_write = 0;
            for (int i = 0; i < count; ++i) {
                const ssize_t rc = ::pwrite(fd, vector[i].iov_base, vector[i].iov_len, offset);
                if (rc <= 0) {
                    return total_write > 0 ? total_write : rc;
                }
                total_write += rc;
                offset += rc;
                if (rc < (ssize_t) vector[i].iov_len) {
                    break;
                }
            }
            return total_write;
        }

#if ARCH_CPU_X86_64

#ifndef SYS_preadv
#define SYS_preadv 295
#endif  // SYS_preadv

#ifndef SYS_pwritev
#define SYS_pwritev 296
#endif // SYS_pwritev

        // SYS_preadv/SYS_pwritev is available since Linux 2.6.30
        static ssize_t sys_preadv(int fd, const struct iovec *vector,
                                  int count, off_t offset) {
            return syscall(SYS_preadv, fd, vector, count, offset);
        }

        static ssize_t sys_pwritev(int fd, const struct iovec *vector,
                                   int count, off_t offset) {
            return syscall(SYS_pwritev, fd, vector, count, offset);
        }

        inline iov_function get_preadv_func() {
#if defined(OS_MACOSX)
            return user_preadv;
#endif
            FDGuard fd(open("/dev/zero", O_RDONLY));
            if (fd < 0) {
                PKLOG(WARNING) << "Fail to open /dev/zero";
                return user_preadv;
            }
            char dummy[1];
            iovec vec = {dummy, sizeof(dummy)};
            const int rc = syscall(SYS_preadv, (int) fd, &vec, 1, 0);
            if (rc < 0) {
                PKLOG(WARNING) << "The kernel doesn't support SYS_preadv, "
                         " use user_preadv instead";
                return user_preadv;
            }
            return sys_preadv;
        }

        inline iov_function get_pwritev_func() {
            FDGuard fd(open("/dev/null", O_WRONLY));
            if (fd < 0) {
                PKLOG(ERROR) << "Fail to open /dev/null";
                return user_pwritev;
            }
#if defined(OS_MACOSX)
            return user_pwritev;
#endif
            char dummy[1];
            iovec vec = {dummy, sizeof(dummy)};
            const int rc = syscall(SYS_pwritev, (int) fd, &vec, 1, 0);
            if (rc < 0) {
                PKLOG(WARNING) << "The kernel doesn't support SYS_pwritev, "
                         " use user_pwritev instead";
                return user_pwritev;
            }
            return sys_pwritev;
        }

#else   // ARCH_CPU_X86_64

#warning "We don't check if the kernel supports SYS_preadv or SYS_pwritev on non-X86_64, use implementation on pread/pwrite directly."

        inline iov_function get_preadv_func() {
            return user_preadv;
        }

        inline iov_function get_pwritev_func() {
            return user_pwritev;
        }

#endif  // ARCH_CPU_X86_64

        void *cp(void *__restrict dest, const void *__restrict src, size_t n) {
            // memcpy in gcc 4.8 seems to be faster enough.
            return ::memcpy(dest, src, n);
        }

        // Function pointers to allocate or deallocate memory for a CordBuffer::Block
        void * (*blockmem_allocate)(size_t) = ::malloc;

        void (*blockmem_deallocate)(void *) = ::free;

        void remove_tls_block_chain();

        // Use default function pointers
        void reset_blockmem_allocate_and_deallocate() {
            // There maybe block allocated by previous hooks, it's wrong to free them using
            // mismatched hook.
            remove_tls_block_chain();
            blockmem_allocate = ::malloc;
            blockmem_deallocate = ::free;
        }

        std::atomic<size_t> g_nblock{0};
        std::atomic<size_t> g_blockmem{0};
        std::atomic<size_t> g_newbigview{0};

        void inc_g_nblock() {
            g_nblock.fetch_add(1, std::memory_order_relaxed);
        }

        void dec_g_nblock() {
            g_nblock.fetch_sub(1, std::memory_order_relaxed);
        }

        void inc_g_blockmem() {
            g_blockmem.fetch_add(1, std::memory_order_relaxed);
        }

        void dec_g_blockmem() {
            g_blockmem.fetch_sub(1, std::memory_order_relaxed);
        }
    } // namespace iobuf

    size_t CordBuffer::block_count() {
        return iobuf::g_nblock.load(std::memory_order_relaxed);
    }

    size_t CordBuffer::block_memory() {
        return iobuf::g_blockmem.load(std::memory_order_relaxed);
    }

    size_t CordBuffer::new_bigview_count() {
        return iobuf::g_newbigview.load(std::memory_order_relaxed);
    }

    namespace iobuf {
        // for unit test
        int block_shared_count(CordBuffer::Block const *b) { return b->ref_count(); }

        CordBuffer::Block *get_portal_next(CordBuffer::Block const *b) {
            return b->u.portal_next;
        }

        uint32_t block_cap(CordBuffer::Block const *b) {
            return b->cap;
        }

        uint32_t block_size(CordBuffer::Block const *b) {
            return b->size;
        }

        inline CordBuffer::Block *create_block_aligned(size_t block_size, size_t alignment) {
            if (block_size > 0xFFFFFFFFULL) {
                KLOG(FATAL) << "block_size=" << block_size << " is too large";
                return nullptr;
            }
            //char *mem = (char *) iobuf::blockmem_allocate(block_size);
            char *mem = (char *) malloc(block_size);
            if (mem == nullptr) {
                return nullptr;
            }
            char *data = mem + sizeof(CordBuffer::Block);
            // change data pointer & data size make align satisfied
            size_t adder = (-reinterpret_cast<uintptr_t>(data)) & (alignment - 1);
            size_t size =
                    (block_size - sizeof(CordBuffer::Block) - adder) & ~(alignment - 1);
            return new(mem) CordBuffer::Block(data + adder, size);
        }

        // === Share TLS blocks between appending operations ===

        static __thread TLSData g_tls_data = {nullptr, 0, false};

        // Used in release_tls_block()
        TLSData *get_g_tls_data() { return &g_tls_data; }
        // Used in UT
        CordBuffer::Block *get_tls_block_head() { return g_tls_data.block_head; }
        int get_tls_block_count() { return g_tls_data.num_blocks; }

        // Number of blocks that can't be returned to TLS which has too many block
        // already. This counter should be 0 in most scenarios, otherwise performance
        // of appending functions in IOPortal may be lowered.
        static std::atomic<size_t> g_num_hit_tls_threshold{0};

        void inc_g_num_hit_tls_threshold() {
            g_num_hit_tls_threshold.fetch_add(1, std::memory_order_relaxed);
        }

        void dec_g_num_hit_tls_threshold() {
            g_num_hit_tls_threshold.fetch_sub(1, std::memory_order_relaxed);
        }

        // Called in UT.
        void remove_tls_block_chain() {
            TLSData &tls_data = g_tls_data;
            CordBuffer::Block *b = tls_data.block_head;
            if (!b) {
                return;
            }
            tls_data.block_head = nullptr;
            int n = 0;
            do {
                CordBuffer::Block *const saved_next = b->u.portal_next;
                b->dec_ref();
                b = saved_next;
                ++n;
            } while (b);
            KCHECK_EQ(n, tls_data.num_blocks);
            tls_data.num_blocks = 0;
        }

        // Get a (non-full) block from TLS.
        // Notice that the block is not removed from TLS.
        CordBuffer::Block *share_tls_block() {
            TLSData &tls_data = g_tls_data;
            CordBuffer::Block *const b = tls_data.block_head;
            if (b != nullptr && !b->full()) {
                return b;
            }
            CordBuffer::Block *new_block = nullptr;
            if (b) {
                new_block = b;
                while (new_block && new_block->full()) {
                    CordBuffer::Block *const saved_next = new_block->u.portal_next;
                    new_block->dec_ref();
                    --tls_data.num_blocks;
                    new_block = saved_next;
                }
            } else if (!tls_data.registered) {
                tls_data.registered = true;
                // Only register atexit at the first time
                turbo::thread_atexit(remove_tls_block_chain);
            }
            if (!new_block) {
                new_block = create_block(); // may be nullptr
                if (new_block) {
                    ++tls_data.num_blocks;
                }
            }
            tls_data.block_head = new_block;
            return new_block;
        }

        // Return chained blocks to TLS.
        // NOTE: b MUST be non-nullptr and all blocks linked SHOULD not be full.
        void release_tls_block_chain(CordBuffer::Block *b) {
            TLSData &tls_data = g_tls_data;
            size_t n = 0;
            if (tls_data.num_blocks >= max_blocks_per_thread()) {
                do {
                    ++n;
                    CordBuffer::Block *const saved_next = b->u.portal_next;
                    b->dec_ref();
                    b = saved_next;
                } while (b);
                g_num_hit_tls_threshold.fetch_add(n, std::memory_order_relaxed);
                return;
            }
            CordBuffer::Block *first_b = b;
            CordBuffer::Block *last_b = nullptr;
            do {
                ++n;
                KCHECK(!b->full());
                if (b->u.portal_next == nullptr) {
                    last_b = b;
                    break;
                }
                b = b->u.portal_next;
            } while (true);
            last_b->u.portal_next = tls_data.block_head;
            tls_data.block_head = first_b;
            tls_data.num_blocks += n;
            if (!tls_data.registered) {
                tls_data.registered = true;
                turbo::thread_atexit(remove_tls_block_chain);
            }
        }

        // Get and remove one (non-full) block from TLS. If TLS is empty, create one.
        CordBuffer::Block *acquire_tls_block() {
            TLSData &tls_data = g_tls_data;
            CordBuffer::Block *b = tls_data.block_head;
            if (!b) {
                return create_block();
            }
            while (b->full()) {
                CordBuffer::Block *const saved_next = b->u.portal_next;
                b->dec_ref();
                tls_data.block_head = saved_next;
                --tls_data.num_blocks;
                b = saved_next;
                if (!b) {
                    return create_block();
                }
            }
            tls_data.block_head = b->u.portal_next;
            --tls_data.num_blocks;
            b->u.portal_next = nullptr;
            return b;
        }

        inline CordBuffer::BlockRef *acquire_blockref_array(size_t cap) {
            iobuf::g_newbigview.fetch_add(1, std::memory_order_relaxed);
            return new CordBuffer::BlockRef[cap];
        }

        inline CordBuffer::BlockRef *acquire_blockref_array() {
            return acquire_blockref_array(CordBuffer::INITIAL_CAP);
        }

        inline void release_blockref_array(CordBuffer::BlockRef *refs, size_t cap) {
            delete[] refs;
        }
    } // namespace iobuf

    size_t CordBuffer::block_count_hit_tls_threshold() {
        return iobuf::g_num_hit_tls_threshold.load(std::memory_order_relaxed);
    }

    static_assert(sizeof(CordBuffer::SmallView) == sizeof(CordBuffer::BigView),
                  "sizeof_small_and_big_view_should_equal");

    static_assert(CordBuffer::DEFAULT_BLOCK_SIZE / 4096 * 4096 == CordBuffer::DEFAULT_BLOCK_SIZE,
                  "sizeof_block_should_be_multiply_of_4096");

    const CordBuffer::Area CordBuffer::INVALID_AREA;

    CordBuffer::CordBuffer(const CordBuffer &rhs) {
        if (rhs._small()) {
            _sv = rhs._sv;
            if (_sv.refs[0].block) {
                _sv.refs[0].block->inc_ref();
            }
            if (_sv.refs[1].block) {
                _sv.refs[1].block->inc_ref();
            }
        } else {
            _bv.magic = -1;
            _bv.start = 0;
            _bv.nref = rhs._bv.nref;
            _bv.cap_mask = rhs._bv.cap_mask;
            _bv.nbytes = rhs._bv.nbytes;
            _bv.refs = iobuf::acquire_blockref_array(_bv.capacity());
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.refs[i] = rhs._bv.ref_at(i);
                _bv.refs[i].block->inc_ref();
            }
        }
    }

    CordBuffer& CordBuffer::operator=(const CordBuffer &rhs) {
        if (this == &rhs) {
            return *this;
        }
        if (!rhs._small() && !_small() && _bv.cap_mask == rhs._bv.cap_mask) {
            // Reuse array of refs
            // Remove references to previous blocks.
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.ref_at(i).block->dec_ref();
            }
            // References blocks in rhs.
            _bv.start = 0;
            _bv.nref = rhs._bv.nref;
            _bv.nbytes = rhs._bv.nbytes;
            for (size_t i = 0; i < _bv.nref; ++i) {
                _bv.refs[i] = rhs._bv.ref_at(i);
                _bv.refs[i].block->inc_ref();
            }
        } else {
            this->~CordBuffer();
            new(this) CordBuffer(rhs);
        }
        return *this;
    }

    template<bool MOVE>
    void CordBuffer::_push_or_move_back_ref_to_smallview(const BlockRef &r) {
        BlockRef *const refs = _sv.refs;
        if (nullptr == refs[0].block) {
            refs[0] = r;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        if (nullptr == refs[1].block) {
            if (refs[0].block == r.block &&
                refs[0].offset + refs[0].length == r.offset) {
                // Merge ref
                refs[0].length += r.length;
                if (MOVE) {
                    r.block->dec_ref();
                }
                return;
            }
            refs[1] = r;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        if (refs[1].block == r.block &&
            refs[1].offset + refs[1].length == r.offset) {
            // Merge ref
            refs[1].length += r.length;
            if (MOVE) {
                r.block->dec_ref();
            }
            return;
        }
        // Convert to BigView
        BlockRef *new_refs = iobuf::acquire_blockref_array();
        new_refs[0] = refs[0];
        new_refs[1] = refs[1];
        new_refs[2] = r;
        const size_t new_nbytes = refs[0].length + refs[1].length + r.length;
        if (!MOVE) {
            r.block->inc_ref();
        }
        _bv.magic = -1;
        _bv.start = 0;
        _bv.refs = new_refs;
        _bv.nref = 3;
        _bv.cap_mask = INITIAL_CAP - 1;
        _bv.nbytes = new_nbytes;
    }

    // Explicitly initialize templates.
    template void CordBuffer::_push_or_move_back_ref_to_smallview<true>(const BlockRef &);

    template void CordBuffer::_push_or_move_back_ref_to_smallview<false>(const BlockRef &);

    template<bool MOVE>
    void CordBuffer::_push_or_move_back_ref_to_bigview(const BlockRef &r) {
        BlockRef &back = _bv.ref_at(_bv.nref - 1);
        if (back.block == r.block && back.offset + back.length == r.offset) {
            // Merge ref
            back.length += r.length;
            _bv.nbytes += r.length;
            if (MOVE) {
                r.block->dec_ref();
            }
            return;
        }

        if (_bv.nref != _bv.capacity()) {
            _bv.ref_at(_bv.nref++) = r;
            _bv.nbytes += r.length;
            if (!MOVE) {
                r.block->inc_ref();
            }
            return;
        }
        // resize, don't modify bv until new_refs is fully assigned
        const uint32_t new_cap = _bv.capacity() * 2;
        BlockRef *new_refs = iobuf::acquire_blockref_array(new_cap);
        for (uint32_t i = 0; i < _bv.nref; ++i) {
            new_refs[i] = _bv.ref_at(i);
        }
        new_refs[_bv.nref++] = r;

        // Change other variables
        _bv.start = 0;
        iobuf::release_blockref_array(_bv.refs, _bv.capacity());
        _bv.refs = new_refs;
        _bv.cap_mask = new_cap - 1;
        _bv.nbytes += r.length;
        if (!MOVE) {
            r.block->inc_ref();
        }
    }

    // Explicitly initialize templates.
    template void CordBuffer::_push_or_move_back_ref_to_bigview<true>(const BlockRef &);

    template void CordBuffer::_push_or_move_back_ref_to_bigview<false>(const BlockRef &);

    template<bool MOVEOUT>
    int CordBuffer::_pop_or_moveout_front_ref() {
        if (_small()) {
            if (_sv.refs[0].block != nullptr) {
                if (!MOVEOUT) {
                    _sv.refs[0].block->dec_ref();
                }
                _sv.refs[0] = _sv.refs[1];
                reset_block_ref(_sv.refs[1]);
                return 0;
            }
            return -1;
        } else {
            // _bv.nref must be greater than 2
            const uint32_t start = _bv.start;
            if (!MOVEOUT) {
                _bv.refs[start].block->dec_ref();
            }
            if (--_bv.nref > 2) {
                _bv.start = (start + 1) & _bv.cap_mask;
                _bv.nbytes -= _bv.refs[start].length;
            } else {
                // count==2, fall back to SmallView
                BlockRef *const saved_refs = _bv.refs;
                const uint32_t saved_cap_mask = _bv.cap_mask;
                _sv.refs[0] = saved_refs[(start + 1) & saved_cap_mask];
                _sv.refs[1] = saved_refs[(start + 2) & saved_cap_mask];
                iobuf::release_blockref_array(saved_refs, saved_cap_mask + 1);
            }
            return 0;
        }
    }

    // Explicitly initialize templates.
    template int CordBuffer::_pop_or_moveout_front_ref<true>();

    template int CordBuffer::_pop_or_moveout_front_ref<false>();

    int CordBuffer::_pop_back_ref() {
        if (_small()) {
            if (_sv.refs[1].block != nullptr) {
                _sv.refs[1].block->dec_ref();
                reset_block_ref(_sv.refs[1]);
                return 0;
            } else if (_sv.refs[0].block != nullptr) {
                _sv.refs[0].block->dec_ref();
                reset_block_ref(_sv.refs[0]);
                return 0;
            }
            return -1;
        } else {
            // _bv.nref must be greater than 2
            const uint32_t start = _bv.start;
            CordBuffer::BlockRef &back = _bv.refs[(start + _bv.nref - 1) & _bv.cap_mask];
            back.block->dec_ref();
            if (--_bv.nref > 2) {
                _bv.nbytes -= back.length;
            } else {
                // count==2, fall back to SmallView
                BlockRef *const saved_refs = _bv.refs;
                const uint32_t saved_cap_mask = _bv.cap_mask;
                _sv.refs[0] = saved_refs[start];
                _sv.refs[1] = saved_refs[(start + 1) & saved_cap_mask];
                iobuf::release_blockref_array(saved_refs, saved_cap_mask + 1);
            }
            return 0;
        }
    }

    void CordBuffer::clear() {
        if (_small()) {
            if (_sv.refs[0].block != nullptr) {
                _sv.refs[0].block->dec_ref();
                reset_block_ref(_sv.refs[0]);

                if (_sv.refs[1].block != nullptr) {
                    _sv.refs[1].block->dec_ref();
                    reset_block_ref(_sv.refs[1]);
                }
            }
        } else {
            for (uint32_t i = 0; i < _bv.nref; ++i) {
                _bv.ref_at(i).block->dec_ref();
            }
            iobuf::release_blockref_array(_bv.refs, _bv.capacity());
            new(this) CordBuffer;
        }
    }

    size_t CordBuffer::pop_front(size_t n) {
        const size_t len = length();
        if (n >= len) {
            clear();
            return len;
        }
        const size_t saved_n = n;
        while (n) {
            // length() == 0 does not enter
            CordBuffer::BlockRef &r = _front_ref();
            if (r.length > n) {
                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
            n -= r.length;
            _pop_front_ref();
        }
        return saved_n;
    }

    bool CordBuffer::cut1(void *c) {
        if (empty()) {
            return false;
        }
        CordBuffer::BlockRef &r = _front_ref();
        *(char *) c = r.block->data[r.offset];
        if (r.length > 1) {
            ++r.offset;
            --r.length;
            if (!_small()) {
                --_bv.nbytes;
            }
        } else {
            _pop_front_ref();
        }
        return true;
    }

    size_t CordBuffer::pop_back(size_t n) {
        const size_t len = length();
        if (n >= len) {
            clear();
            return len;
        }
        const size_t saved_n = n;
        while (n) {
            // length() == 0 does not enter
            CordBuffer::BlockRef &r = _back_ref();
            if (r.length > n) {
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
            n -= r.length;
            _pop_back_ref();
        }
        return saved_n;
    }

    size_t CordBuffer::cutn(CordBuffer *out, size_t n) {
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t saved_n = n;
        while (n) {
            // length() == 0 does not enter
            CordBuffer::BlockRef &r = _front_ref();
            if (r.length <= n) {
                n -= r.length;
                out->_move_back_ref(r);
                _moveout_front_ref();
            } else {
                const CordBuffer::BlockRef cr = {r.offset, (uint32_t) n, r.block};
                out->_push_back_ref(cr);

                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
        }
        return saved_n;
    }

    size_t CordBuffer::cutn(void *out, size_t n) {
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t saved_n = n;
        while (n) {
            // length() == 0 does not enter
            CordBuffer::BlockRef &r = _front_ref();
            if (r.length <= n) {
                iobuf::cp(out, r.block->data + r.offset, r.length);
                out = (char *) out + r.length;
                n -= r.length;
                _pop_front_ref();
            } else {
                iobuf::cp(out, r.block->data + r.offset, n);
                out = (char *) out + n;
                r.offset += n;
                r.length -= n;
                if (!_small()) {
                    _bv.nbytes -= n;
                }
                return saved_n;
            }
        }
        return saved_n;
    }

    size_t CordBuffer::cutn(std::string *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t len = length();
        if (n > len) {
            n = len;
        }
        const size_t old_size = out->size();
        out->resize(out->size() + n);
        return cutn(&(*out)[old_size], n);
    }

    int CordBuffer::_cut_by_char(CordBuffer *out, char d) {
        const size_t nref = _ref_num();
        size_t n = 0;

        for (size_t i = 0; i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            char const *const s = r.block->data + r.offset;
            for (uint32_t j = 0; j < r.length; ++j, ++n) {
                if (s[j] == d) {
                    // There's no way cutn/pop_front fails
                    cutn(out, n);
                    pop_front(1);
                    return 0;
                }
            }
        }

        return -1;
    }

    int CordBuffer::_cut_by_delim(CordBuffer *out, char const *dbegin, size_t ndelim) {
        typedef unsigned long SigType;
        const size_t NMAX = sizeof(SigType);

        if (ndelim > NMAX || ndelim > length()) {
            return -1;
        }

        SigType dsig = 0;
        for (size_t i = 0; i < ndelim; ++i) {
            dsig = (dsig << CHAR_BIT) | static_cast<SigType>(dbegin[i]);
        }

        const SigType SIGMASK =
                (ndelim == NMAX ? (SigType) -1 : (((SigType) 1 << (ndelim * CHAR_BIT)) - 1));

        const size_t nref = _ref_num();
        SigType sig = 0;
        size_t n = 0;

        for (size_t i = 0; i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            char const *const s = r.block->data + r.offset;

            for (uint32_t j = 0; j < r.length; ++j, ++n) {
                sig = ((sig << CHAR_BIT) | static_cast<SigType>(s[j])) & SIGMASK;
                if (sig == dsig) {
                    // There's no way cutn/pop_front fails
                    cutn(out, n + 1 - ndelim);
                    pop_front(ndelim);
                    return 0;
                }
            }
        }

        return -1;
    }

    // Since cut_into_file_descriptor() allocates iovec on stack, IOV_MAX=1024
    // is too large(in the worst case) for bthreads with small stacks.
    static const size_t IOBUF_IOV_MAX = 256;

    ssize_t CordBuffer::pcut_into_file_descriptor(int fd, off_t offset, size_t size_hint) {
        if (empty()) {
            return 0;
        }

        const size_t nref = std::min(_ref_num(), IOBUF_IOV_MAX);
        struct iovec vec[nref];
        size_t nvec = 0;
        size_t cur_len = 0;

        do {
            CordBuffer::BlockRef const &r = _ref_at(nvec);
            vec[nvec].iov_base = r.block->data + r.offset;
            vec[nvec].iov_len = r.length;
            ++nvec;
            cur_len += r.length;
        } while (nvec < nref && cur_len < size_hint);

        ssize_t nw = 0;

        if (offset >= 0) {
            static iobuf::iov_function pwritev_func = iobuf::get_pwritev_func();
            nw = pwritev_func(fd, vec, nvec, offset);
        } else {
            nw = ::writev(fd, vec, nvec);
        }
        if (nw > 0) {
            pop_front(nw);
        }
        return nw;
    }

    ssize_t CordBuffer::cut_into_writer(IWriter *writer, size_t size_hint) {
        if (empty()) {
            return 0;
        }
        const size_t nref = std::min(_ref_num(), IOBUF_IOV_MAX);
        struct iovec vec[nref];
        size_t nvec = 0;
        size_t cur_len = 0;

        do {
            CordBuffer::BlockRef const &r = _ref_at(nvec);
            vec[nvec].iov_base = r.block->data + r.offset;
            vec[nvec].iov_len = r.length;
            ++nvec;
            cur_len += r.length;
        } while (nvec < nref && cur_len < size_hint);

        const ssize_t nw = writer->WriteV(vec, nvec);
        if (nw > 0) {
            pop_front(nw);
        }
        return nw;
    }

    ssize_t CordBuffer::cut_into_SSL_channel(SSL *ssl, int *ssl_error) {
        *ssl_error = SSL_ERROR_NONE;
        if (empty()) {
            return 0;
        }

        CordBuffer::BlockRef const &r = _ref_at(0);
        ERR_clear_error();
        const int nw = SSL_write(ssl, r.block->data + r.offset, r.length);
        if (nw > 0) {
            pop_front(nw);
        }
        *ssl_error = SSL_get_error(ssl, nw);
        return nw;
    }

    ssize_t CordBuffer::cut_multiple_into_SSL_channel(SSL *ssl, CordBuffer *const*pieces,
                                                      size_t count, int *ssl_error) {
        ssize_t nw = 0;
        *ssl_error = SSL_ERROR_NONE;
        for (size_t i = 0; i < count;) {
            if (pieces[i]->empty()) {
                ++i;
                continue;
            }

            ssize_t rc = pieces[i]->cut_into_SSL_channel(ssl, ssl_error);
            if (rc > 0) {
                nw += rc;
            } else {
                if (rc < 0) {
                    if (*ssl_error == SSL_ERROR_WANT_WRITE
                        || (*ssl_error == SSL_ERROR_SYSCALL
                            && BIO_fd_non_fatal_error(errno) == 1)) {
                        // Non fatal error, tell caller to write again
                        *ssl_error = SSL_ERROR_WANT_WRITE;
                    } else {
                        // Other errors are fatal
                        return rc;
                    }
                }
                if (nw == 0) {
                    nw = rc; // Nothing written yet, overwrite nw
                }
                break;
            }
        }

#ifndef USE_MESALINK
        // BIO is disabled for now (see socket.cpp) and the following implementation is
        // NOT correct since it doesn't handle the EAGAIN event of BIO_flush
        //    BIO* wbio = SSL_get_wbio(ssl);
        //    if (BIO_wpending(wbio) > 0) {
        //        int rc = BIO_flush(wbio);
        //        if (rc <= 0 && BIO_fd_non_fatal_error(errno) == 0) {
        //            // Fatal error during BIO_flush
        //            *ssl_error = SSL_ERROR_SYSCALL;
        //            return rc;
        //        }
        //    }
#else
        int rc = SSL_flush(ssl);
        if (rc <= 0) {
            *ssl_error = SSL_ERROR_SYSCALL;
            return rc;
        }
#endif

        return nw;
    }

    ssize_t CordBuffer::pcut_multiple_into_file_descriptor(
        int fd, off_t offset, CordBuffer *const*pieces, size_t count) {
        if (TURBO_UNLIKELY(count == 0)) {
            return 0;
        }
        if (1UL == count) {
            return pieces[0]->pcut_into_file_descriptor(fd, offset);
        }
        struct iovec vec[IOBUF_IOV_MAX];
        size_t nvec = 0;
        for (size_t i = 0; i < count; ++i) {
            const CordBuffer *p = pieces[i];
            const size_t nref = p->_ref_num();
            for (size_t j = 0; j < nref && nvec < IOBUF_IOV_MAX; ++j, ++nvec) {
                CordBuffer::BlockRef const &r = p->_ref_at(j);
                vec[nvec].iov_base = r.block->data + r.offset;
                vec[nvec].iov_len = r.length;
            }
        }

        ssize_t nw = 0;
        if (offset >= 0) {
            static iobuf::iov_function pwritev_func = iobuf::get_pwritev_func();
            nw = pwritev_func(fd, vec, nvec, offset);
        } else {
            nw = ::writev(fd, vec, nvec);
        }
        if (nw <= 0) {
            return nw;
        }
        size_t npop_all = nw;
        for (size_t i = 0; i < count; ++i) {
            npop_all -= pieces[i]->pop_front(npop_all);
            if (npop_all == 0) {
                break;
            }
        }
        return nw;
    }

    ssize_t CordBuffer::cut_multiple_into_writer(
        IWriter *writer, CordBuffer *const*pieces, size_t count) {
        if (TURBO_UNLIKELY(count == 0)) {
            return 0;
        }
        if (1UL == count) {
            return pieces[0]->cut_into_writer(writer);
        }
        struct iovec vec[IOBUF_IOV_MAX];
        size_t nvec = 0;
        for (size_t i = 0; i < count; ++i) {
            const CordBuffer *p = pieces[i];
            const size_t nref = p->_ref_num();
            for (size_t j = 0; j < nref && nvec < IOBUF_IOV_MAX; ++j, ++nvec) {
                CordBuffer::BlockRef const &r = p->_ref_at(j);
                vec[nvec].iov_base = r.block->data + r.offset;
                vec[nvec].iov_len = r.length;
            }
        }

        const ssize_t nw = writer->WriteV(vec, nvec);
        if (nw <= 0) {
            return nw;
        }
        size_t npop_all = nw;
        for (size_t i = 0; i < count; ++i) {
            npop_all -= pieces[i]->pop_front(npop_all);
            if (npop_all == 0) {
                break;
            }
        }
        return nw;
    }


    void CordBuffer::append(const CordBuffer &other) {
        const size_t nref = other._ref_num();
        for (size_t i = 0; i < nref; ++i) {
            _push_back_ref(other._ref_at(i));
        }
    }

    void CordBuffer::append(const Movable &movable_other) {
        if (empty()) {
            swap(movable_other.value());
        } else {
            CordBuffer &other = movable_other.value();
            const size_t nref = other._ref_num();
            for (size_t i = 0; i < nref; ++i) {
                _move_back_ref(other._ref_at(i));
            }
            if (!other._small()) {
                iobuf::release_blockref_array(other._bv.refs, other._bv.capacity());
            }
            new(&other) CordBuffer;
        }
    }

    int CordBuffer::push_back(char c) {
        CordBuffer::Block *b = iobuf::share_tls_block();
        if (TURBO_UNLIKELY(!b)) {
            return -1;
        }
        b->data[b->size] = c;
        const CordBuffer::BlockRef r = {b->size, 1, b};
        ++b->size;
        _push_back_ref(r);
        return 0;
    }


    int CordBuffer::append(void const *data, size_t count) {
        if (TURBO_UNLIKELY(!data)) {
            return -1;
        }
        if (count == 1) {
            return push_back(*((char const *) data));
        }
        size_t total_nc = 0;
        while (total_nc < count) {
            // excluded count == 0
            CordBuffer::Block *b = iobuf::share_tls_block();
            if (TURBO_UNLIKELY(!b)) {
                return -1;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            iobuf::cp(b->data + b->size, (char *) data + total_nc, nc);

            const CordBuffer::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            b->size += nc;
            total_nc += nc;
        }
        return 0;
    }

    int CordBuffer::appendv(const fconst_iovec *vec, size_t n) {
        size_t offset = 0;
        for (size_t i = 0; i < n;) {
            CordBuffer::Block *b = iobuf::share_tls_block();
            if (TURBO_UNLIKELY(!b)) {
                return -1;
            }
            uint32_t total_cp = 0;
            for (; i < n; ++i, offset = 0) {
                const fconst_iovec &vec_i = vec[i];
                const size_t nc = std::min(vec_i.iov_len - offset, b->left_space() - total_cp);
                iobuf::cp(b->data + b->size + total_cp, (char *) vec_i.iov_base + offset, nc);
                total_cp += nc;
                offset += nc;
                if (offset != vec_i.iov_len) {
                    break;
                }
            }

            const CordBuffer::BlockRef r = {(uint32_t) b->size, total_cp, b};
            b->size += total_cp;
            _push_back_ref(r);
        }
        return 0;
    }

    int CordBuffer::append_user_data_with_meta(void *data,
                                               size_t size,
                                               std::function<void(void *)> deleter,
                                               uint64_t meta) {
        if (size > 0xFFFFFFFFULL - 100) {
            KLOG(FATAL) << "data_size=" << size << " is too large";
            return -1;
        }
        if (!deleter) {
            deleter = ::free;
        }
        if (!size) {
            deleter(data);
            return 0;
        }
        char *mem = (char *) malloc(sizeof(CordBuffer::Block) + sizeof(UserDataExtension));
        if (mem == nullptr) {
            return -1;
        }
        CordBuffer::Block *b = new(mem) CordBuffer::Block((char *) data, size, std::move(deleter));
        b->u.data_meta = meta;
        const CordBuffer::BlockRef r = {0, b->cap, b};
        _move_back_ref(r);
        return 0;
    }

    uint64_t CordBuffer::get_first_data_meta() {
        if (_ref_num() == 0) {
            return 0;
        }
        CordBuffer::BlockRef const &r = _ref_at(0);
        if (!(r.block->flags & IOBUF_BLOCK_FLAGS_USER_DATA)) {
            return 0;
        }
        return r.block->u.data_meta;
    }

    int CordBuffer::resize(size_t n, char c) {
        const size_t saved_len = length();
        if (n < saved_len) {
            pop_back(saved_len - n);
            return 0;
        }
        const size_t count = n - saved_len;
        size_t total_nc = 0;
        while (total_nc < count) {
            // excluded count == 0
            CordBuffer::Block *b = iobuf::share_tls_block();
            if (TURBO_UNLIKELY(!b)) {
                return -1;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            memset(b->data + b->size, c, nc);

            const CordBuffer::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            b->size += nc;
            total_nc += nc;
        }
        return 0;
    }

    // NOTE: We don't use C++ bitwise fields which make copying slower.
    static const int REF_INDEX_BITS = 19;
    static const int REF_OFFSET_BITS = 15;
    static const int AREA_SIZE_BITS = 30;
    static const uint32_t MAX_REF_INDEX = (((uint32_t) 1) << REF_INDEX_BITS) - 1;
    static const uint32_t MAX_REF_OFFSET = (((uint32_t) 1) << REF_OFFSET_BITS) - 1;
    static const uint32_t MAX_AREA_SIZE = (((uint32_t) 1) << AREA_SIZE_BITS) - 1;

    inline CordBuffer::Area make_area(uint32_t ref_index, uint32_t ref_offset,
                                      uint32_t size) {
        if (ref_index > MAX_REF_INDEX ||
            ref_offset > MAX_REF_OFFSET ||
            size > MAX_AREA_SIZE) {
            KLOG(ERROR) << "Too big parameters!";
            return CordBuffer::INVALID_AREA;
        }
        return (((uint64_t) ref_index) << (REF_OFFSET_BITS + AREA_SIZE_BITS))
               | (((uint64_t) ref_offset) << AREA_SIZE_BITS)
               | size;
    }

    inline uint32_t get_area_ref_index(CordBuffer::Area c) {
        return (c >> (REF_OFFSET_BITS + AREA_SIZE_BITS)) & MAX_REF_INDEX;
    }

    inline uint32_t get_area_ref_offset(CordBuffer::Area c) {
        return (c >> AREA_SIZE_BITS) & MAX_REF_OFFSET;
    }

    inline uint32_t get_area_size(CordBuffer::Area c) {
        return (c & MAX_AREA_SIZE);
    }

    CordBuffer::Area CordBuffer::reserve(size_t count) {
        CordBuffer::Area result = INVALID_AREA;
        size_t total_nc = 0;
        while (total_nc < count) {
            // excluded count == 0
            CordBuffer::Block *b = iobuf::share_tls_block();
            if (TURBO_UNLIKELY(!b)) {
                return INVALID_AREA;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            const CordBuffer::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            if (total_nc == 0) {
                // Encode the area at first time. Notice that the pushed ref may
                // be merged with existing ones.
                result = make_area(_ref_num() - 1, _back_ref().length - nc, count);
            }
            total_nc += nc;
            b->size += nc;
        }
        return result;
    }

    int CordBuffer::unsafe_assign(Area area, const void *data) {
        if (area == INVALID_AREA || data == nullptr) {
            KLOG(ERROR) << "Invalid parameters";
            return -1;
        }
        const uint32_t ref_index = get_area_ref_index(area);
        uint32_t ref_offset = get_area_ref_offset(area);
        uint32_t length = get_area_size(area);
        const size_t nref = _ref_num();
        for (size_t i = ref_index; i < nref; ++i) {
            CordBuffer::BlockRef &r = _ref_at(i);
            // NOTE: we can't check if the block is shared with another CordBuffer or
            // not since even a single CordBuffer may reference a block multiple times
            // (by different BlockRef-s)

            const size_t nc = std::min(length, r.length - ref_offset);
            iobuf::cp(r.block->data + r.offset + ref_offset, data, nc);
            if (length == nc) {
                return 0;
            }
            ref_offset = 0;
            length -= nc;
            data = (char *) data + nc;
        }

        // Use check because we need to see the stack here.
        KCHECK(false) << "CordBuffer(" << size() << ", nref=" << _ref_num()
                 << ") is shorter than what we reserved("
                 << "ref=" << get_area_ref_index(area)
                 << " off=" << get_area_ref_offset(area)
                 << " size=" << get_area_size(area)
                 << "), this assignment probably corrupted something...";
        return -1;
    }

    size_t CordBuffer::append_to(CordBuffer *buf, size_t n, size_t pos) const {
        const size_t nref = _ref_num();
        // Skip `pos' bytes. `offset' is the starting position in starting BlockRef.
        size_t offset = pos;
        size_t i = 0;
        for (; offset != 0 && i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            if (offset < (size_t) r.length) {
                break;
            }
            offset -= r.length;
        }
        size_t m = n;
        for (; m != 0 && i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            const size_t nc = std::min(m, (size_t) r.length - offset);
            const CordBuffer::BlockRef r2 = {
                (uint32_t) (r.offset + offset),
                (uint32_t) nc, r.block
            };
            buf->_push_back_ref(r2);
            offset = 0;
            m -= nc;
        }
        // If nref == 0, here returns 0 correctly
        return n - m;
    }

    size_t CordBuffer::copy_to(void *d, size_t n, size_t pos) const {
        const size_t nref = _ref_num();
        // Skip `pos' bytes. `offset' is the starting position in starting BlockRef.
        size_t offset = pos;
        size_t i = 0;
        for (; offset != 0 && i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            if (offset < (size_t) r.length) {
                break;
            }
            offset -= r.length;
        }
        size_t m = n;
        for (; m != 0 && i < nref; ++i) {
            CordBuffer::BlockRef const &r = _ref_at(i);
            const size_t nc = std::min(m, (size_t) r.length - offset);
            iobuf::cp(d, r.block->data + r.offset + offset, nc);
            offset = 0;
            d = (char *) d + nc;
            m -= nc;
        }
        // If nref == 0, here returns 0 correctly
        return n - m;
    }

    size_t CordBuffer::copy_to(std::string *s, size_t n, size_t pos) const {
        const size_t len = length();
        if (len <= pos) {
            return 0;
        }
        if (n > len - pos) {
            // note: n + pos may overflow
            n = len - pos;
        }
        s->resize(n);
        return copy_to(&(*s)[0], n, pos);
    }

    size_t CordBuffer::append_to(std::string *s, size_t n, size_t pos) const {
        const size_t len = length();
        if (len <= pos) {
            return 0;
        }
        if (n > len - pos) {
            // note: n + pos may overflow
            n = len - pos;
        }
        const size_t old_size = s->size();
        s->resize(old_size + n);
        return copy_to(&(*s)[old_size], n, pos);
    }


    size_t CordBuffer::copy_to_cstr(char *s, size_t n, size_t pos) const {
        const size_t nc = copy_to(s, n, pos);
        s[nc] = '\0';
        return nc;
    }

    void const *CordBuffer::fetch(void *d, size_t n) const {
        if (n <= length()) {
            CordBuffer::BlockRef const &r0 = _ref_at(0);
            if (n <= r0.length) {
                return r0.block->data + r0.offset;
            }

            iobuf::cp(d, r0.block->data + r0.offset, r0.length);
            size_t total_nc = r0.length;
            const size_t nref = _ref_num();
            for (size_t i = 1; i < nref; ++i) {
                CordBuffer::BlockRef const &r = _ref_at(i);
                if (n <= r.length + total_nc) {
                    iobuf::cp((char *) d + total_nc,
                              r.block->data + r.offset, n - total_nc);
                    return d;
                }
                iobuf::cp((char *) d + total_nc, r.block->data + r.offset, r.length);
                total_nc += r.length;
            }
        }
        return nullptr;
    }

    const void *CordBuffer::fetch1() const {
        if (!empty()) {
            const CordBuffer::BlockRef &r0 = _front_ref();
            return r0.block->data + r0.offset;
        }
        return nullptr;
    }

    std::ostream &operator<<(std::ostream &os, const CordBuffer &buf) {
        const size_t n = buf.backing_block_num();
        for (size_t i = 0; i < n; ++i) {
            auto blk = buf.backing_block(i);
            os.write(blk.data(), blk.size());
        }
        return os;
    }

    bool CordBuffer::equals(std::string_view s) const {
        if (size() != s.size()) {
            return false;
        }
        const size_t nref = _ref_num();
        size_t soff = 0;
        for (size_t i = 0; i < nref; ++i) {
            const BlockRef &r = _ref_at(i);
            if (memcmp(r.block->data + r.offset, s.data() + soff, r.length) != 0) {
                return false;
            }
            soff += r.length;
        }
        return true;
    }

    std::string_view CordBuffer::backing_block(size_t i) const {
        if (i < _ref_num()) {
            const BlockRef &r = _ref_at(i);
            return std::string_view(r.block->data + r.offset, r.length);
        }
        return std::string_view{};
    }

    bool CordBuffer::equals(const CordBuffer &other) const {
        const size_t sz1 = size();
        if (sz1 != other.size()) {
            return false;
        }
        if (!sz1) {
            return true;
        }
        const BlockRef &r1 = _ref_at(0);
        const char *d1 = r1.block->data + r1.offset;
        size_t len1 = r1.length;
        const BlockRef &r2 = other._ref_at(0);
        const char *d2 = r2.block->data + r2.offset;
        size_t len2 = r2.length;
        const size_t nref1 = _ref_num();
        const size_t nref2 = other._ref_num();
        size_t i = 1;
        size_t j = 1;
        do {
            const size_t cmplen = std::min(len1, len2);
            if (memcmp(d1, d2, cmplen) != 0) {
                return false;
            }
            len1 -= cmplen;
            if (!len1) {
                if (i >= nref1) {
                    return true;
                }
                const BlockRef &r = _ref_at(i++);
                d1 = r.block->data + r.offset;
                len1 = r.length;
            } else {
                d1 += cmplen;
            }
            len2 -= cmplen;
            if (!len2) {
                if (j >= nref2) {
                    return true;
                }
                const BlockRef &r = other._ref_at(j++);
                d2 = r.block->data + r.offset;
                len2 = r.length;
            } else {
                d2 += cmplen;
            }
        } while (true);
        return true;
    }

    ////////////////////////////// IOPortal //////////////////
    IOPortal::~IOPortal() { return_cached_blocks(); }

    IOPortal &IOPortal::operator=(const IOPortal &rhs) {
        CordBuffer::operator=(rhs);
        return *this;
    }

    void IOPortal::clear() {
        CordBuffer::clear();
        return_cached_blocks();
    }

    const int MAX_APPEND_IOVEC = 64;

    ssize_t IOPortal::pappend_from_file_descriptor(
        int fd, off_t offset, size_t max_count) {
        iovec vec[MAX_APPEND_IOVEC];
        int nvec = 0;
        size_t space = 0;
        Block *prev_p = nullptr;
        Block *p = _block;
        // Prepare at most MAX_APPEND_IOVEC blocks or space of blocks >= max_count
        do {
            if (p == nullptr) {
                p = iobuf::acquire_tls_block();
                if (TURBO_UNLIKELY(!p)) {
                    errno = ENOMEM;
                    return -1;
                }
                if (prev_p != nullptr) {
                    prev_p->u.portal_next = p;
                } else {
                    _block = p;
                }
            }
            vec[nvec].iov_base = p->data + p->size;
            vec[nvec].iov_len = std::min(p->left_space(), max_count - space);
            space += vec[nvec].iov_len;
            ++nvec;
            if (space >= max_count || nvec >= MAX_APPEND_IOVEC) {
                break;
            }
            prev_p = p;
            p = p->u.portal_next;
        } while (1);

        ssize_t nr = 0;
        if (offset < 0) {
            nr = readv(fd, vec, nvec);
        } else {
            static iobuf::iov_function preadv_func = iobuf::get_preadv_func();
            nr = preadv_func(fd, vec, nvec, offset);
        }
        if (nr <= 0) {
            // -1 or 0
            if (empty()) {
                return_cached_blocks();
            }
            return nr;
        }

        size_t total_len = nr;
        do {
            const size_t len = std::min(total_len, _block->left_space());
            total_len -= len;
            const CordBuffer::BlockRef r = {_block->size, (uint32_t) len, _block};
            _push_back_ref(r);
            _block->size += len;
            if (_block->full()) {
                Block *const saved_next = _block->u.portal_next;
                _block->dec_ref(); // _block may be deleted
                _block = saved_next;
            }
        } while (total_len);
        return nr;
    }

    ssize_t IOPortal::append_from_reader(IReader *reader, size_t max_count) {
        iovec vec[MAX_APPEND_IOVEC];
        int nvec = 0;
        size_t space = 0;
        Block *prev_p = nullptr;
        Block *p = _block;
        // Prepare at most MAX_APPEND_IOVEC blocks or space of blocks >= max_count
        do {
            if (p == nullptr) {
                p = iobuf::acquire_tls_block();
                if (TURBO_UNLIKELY(!p)) {
                    errno = ENOMEM;
                    return -1;
                }
                if (prev_p != nullptr) {
                    prev_p->u.portal_next = p;
                } else {
                    _block = p;
                }
            }
            vec[nvec].iov_base = p->data + p->size;
            vec[nvec].iov_len = std::min(p->left_space(), max_count - space);
            space += vec[nvec].iov_len;
            ++nvec;
            if (space >= max_count || nvec >= MAX_APPEND_IOVEC) {
                break;
            }
            prev_p = p;
            p = p->u.portal_next;
        } while (1);

        const ssize_t nr = reader->ReadV(vec, nvec);
        if (nr <= 0) {
            // -1 or 0
            if (empty()) {
                return_cached_blocks();
            }
            return nr;
        }

        size_t total_len = nr;
        do {
            const size_t len = std::min(total_len, _block->left_space());
            total_len -= len;
            const CordBuffer::BlockRef r = {_block->size, (uint32_t) len, _block};
            _push_back_ref(r);
            _block->size += len;
            if (_block->full()) {
                Block *const saved_next = _block->u.portal_next;
                _block->dec_ref(); // _block may be deleted
                _block = saved_next;
            }
        } while (total_len);
        return nr;
    }


    ssize_t IOPortal::append_from_SSL_channel(
        SSL *ssl, int *ssl_error, size_t max_count) {
        size_t nr = 0;
        do {
            if (!_block) {
                _block = iobuf::acquire_tls_block();
                if (TURBO_UNLIKELY(!_block)) {
                    errno = ENOMEM;
                    *ssl_error = SSL_ERROR_SYSCALL;
                    return -1;
                }
            }

            const size_t read_len = std::min(_block->left_space(), max_count - nr);
            ERR_clear_error();
            const int rc = SSL_read(ssl, _block->data + _block->size, read_len);
            *ssl_error = SSL_get_error(ssl, rc);
            if (rc > 0) {
                const CordBuffer::BlockRef r = {(uint32_t) _block->size, (uint32_t) rc, _block};
                _push_back_ref(r);
                _block->size += rc;
                if (_block->full()) {
                    Block *const saved_next = _block->u.portal_next;
                    _block->dec_ref(); // _block may be deleted
                    _block = saved_next;
                }
                nr += rc;
            } else {
                if (rc < 0) {
                    if (*ssl_error == SSL_ERROR_WANT_READ
                        || (*ssl_error == SSL_ERROR_SYSCALL
                            && BIO_fd_non_fatal_error(errno) == 1)) {
                        // Non fatal error, tell caller to read again
                        *ssl_error = SSL_ERROR_WANT_READ;
                    } else {
                        // Other errors are fatal
                        return rc;
                    }
                }
                return (nr > 0 ? nr : rc);
            }
        } while (nr < max_count);
        return nr;
    }

    void IOPortal::return_cached_blocks_impl(Block *b) {
        iobuf::release_tls_block_chain(b);
    }

    CordBuffer::Area IOReserveAlignedBuf::reserve(size_t count) {
        CordBuffer::Area result = INVALID_AREA;
        if (_reserved == true) {
            KLOG(ERROR) << "Already call reserved";
            return result;
        }
        _reserved = true;
        bool is_power_two = _alignment > 0 && (_alignment & (_alignment - 1));
        if (is_power_two != 0) {
            KLOG(ERROR) << "Invalid alignment, must power of two";
            return INVALID_AREA;
        }
        count = (count + _alignment - 1) & ~(_alignment - 1);
        size_t total_nc = 0;
        while (total_nc < count) {
            auto block_size =
                    std::max(_alignment, 4096UL) * 2 + sizeof(CordBuffer::Block);
            if (turbo::get_flag(FLAGS_iobuf_aligned_buf_block_size) != 0) {
                block_size = turbo::get_flag(FLAGS_iobuf_aligned_buf_block_size);
            }
            auto b = iobuf::create_block_aligned(block_size, _alignment);
            if (TURBO_UNLIKELY(!b)) {
                KLOG(ERROR) << "Create block failed";
                return result;
            }
            const size_t nc = std::min(count - total_nc, b->left_space());
            const CordBuffer::BlockRef r = {(uint32_t) b->size, (uint32_t) nc, b};
            _push_back_ref(r);
            // aligned block is not from tls, release block ref
            b->dec_ref();
            if (total_nc == 0) {
                // Encode the area at first time. Notice that the pushed ref may
                // be merged with existing ones.
                result = make_area(_ref_num() - 1, _back_ref().length - nc, count);
            }
            // add total nc
            total_nc += nc;
            b->size += nc;
        };
        return result;
    }

    //////////////// IOBufCutter ////////////////

    IOBufCutter::IOBufCutter(CordBuffer *buf)
        : _data(nullptr)
          , _data_end(nullptr)
          , _block(nullptr)
          , _buf(buf) {
    }

    IOBufCutter::~IOBufCutter() {
        if (_block) {
            if (_data != _data_end) {
                CordBuffer::BlockRef &fr = _buf->_front_ref();
                KCHECK_EQ(fr.block, _block);
                fr.offset = (uint32_t) ((char *) _data - _block->data);
                fr.length = (uint32_t) ((char *) _data_end - (char *) _data);
            } else {
                _buf->_pop_front_ref();
            }
        }
    }

    bool IOBufCutter::load_next_ref() {
        if (_block) {
            _buf->_pop_front_ref();
        }
        if (!_buf->_ref_num()) {
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return false;
        } else {
            const CordBuffer::BlockRef &r = _buf->_front_ref();
            _data = r.block->data + r.offset;
            _data_end = (char *) _data + r.length;
            _block = r.block;
            return true;
        }
    }

    size_t IOBufCutter::slower_copy_to(void *dst, size_t n) {
        size_t size = (char *) _data_end - (char *) _data;
        if (size == 0) {
            if (!load_next_ref()) {
                return 0;
            }
            size = (char *) _data_end - (char *) _data;
            if (n <= size) {
                memcpy(dst, _data, n);
                return n;
            }
        }
        void *const saved_dst = dst;
        memcpy(dst, _data, size);
        dst = (char *) dst + size;
        n -= size;
        const size_t nref = _buf->_ref_num();
        for (size_t i = 1; i < nref; ++i) {
            const CordBuffer::BlockRef &r = _buf->_ref_at(i);
            const size_t nc = std::min(n, (size_t) r.length);
            memcpy(dst, r.block->data + r.offset, nc);
            dst = (char *) dst + nc;
            n -= nc;
            if (n == 0) {
                break;
            }
        }
        return (char *) dst - (char *) saved_dst;
    }

    size_t IOBufCutter::cutn(CordBuffer *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t size = (char *) _data_end - (char *) _data;
        if (n <= size) {
            const CordBuffer::BlockRef r = {
                (uint32_t) ((char *) _data - _block->data),
                (uint32_t) n,
                _block
            };
            out->_push_back_ref(r);
            _data = (char *) _data + n;
            return n;
        } else if (size != 0) {
            const CordBuffer::BlockRef r = {
                (uint32_t) ((char *) _data - _block->data),
                (uint32_t) size,
                _block
            };
            out->_push_back_ref(r);
            _buf->_pop_front_ref();
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return _buf->cutn(out, n - size) + size;
        } else {
            if (_block) {
                _data = nullptr;
                _data_end = nullptr;
                _block = nullptr;
                _buf->_pop_front_ref();
            }
            return _buf->cutn(out, n);
        }
    }

    size_t IOBufCutter::cutn(void *out, size_t n) {
        if (n == 0) {
            return 0;
        }
        const size_t size = (char *) _data_end - (char *) _data;
        if (n <= size) {
            memcpy(out, _data, n);
            _data = (char *) _data + n;
            return n;
        } else if (size != 0) {
            memcpy(out, _data, size);
            _buf->_pop_front_ref();
            _data = nullptr;
            _data_end = nullptr;
            _block = nullptr;
            return _buf->cutn((char *) out + size, n - size) + size;
        } else {
            if (_block) {
                _data = nullptr;
                _data_end = nullptr;
                _block = nullptr;
                _buf->_pop_front_ref();
            }
            return _buf->cutn(out, n);
        }
    }

    IOBufAsZeroCopyInputStream::IOBufAsZeroCopyInputStream(const CordBuffer &buf)
        : _ref_index(0)
          , _add_offset(0)
          , _byte_count(0)
          , _buf(&buf) {
    }

    bool IOBufAsZeroCopyInputStream::Next(const void **data, int *size) {
        const CordBuffer::BlockRef *cur_ref = _buf->_pref_at(_ref_index);
        if (cur_ref == nullptr) {
            return false;
        }
        *data = cur_ref->block->data + cur_ref->offset + _add_offset;
        // Impl. of Backup/Skip guarantees that _add_offset < cur_ref->length.
        *size = cur_ref->length - _add_offset;
        _byte_count += cur_ref->length - _add_offset;
        _add_offset = 0;
        ++_ref_index;
        return true;
    }

    void IOBufAsZeroCopyInputStream::BackUp(int count) {
        if (_ref_index > 0) {
            const CordBuffer::BlockRef *cur_ref = _buf->_pref_at(--_ref_index);
            KCHECK(_add_offset == 0 && cur_ref->length >= (uint32_t)count)
            << "BackUp() is not after a Next()";
            _add_offset = cur_ref->length - count;
            _byte_count -= count;
        } else {
            KLOG(FATAL) << "BackUp an empty ZeroCopyInputStream";
        }
    }

    // Skips `count` number of bytes.
    // Returns true on success, or false if some input error occurred, or `count`
    // exceeds the end of the stream. This function may skip up to `count - 1`
    // bytes in case of failure.
    //
    // Preconditions:
    // * `count` is non-negative.
    //
    bool IOBufAsZeroCopyInputStream::Skip(int count) {
        const CordBuffer::BlockRef *cur_ref = _buf->_pref_at(_ref_index);
        while (cur_ref) {
            const int left_bytes = cur_ref->length - _add_offset;
            if (count < left_bytes) {
                _add_offset += count;
                _byte_count += count;
                return true;
            }
            count -= left_bytes;
            _add_offset = 0;
            _byte_count += left_bytes;
            cur_ref = _buf->_pref_at(++_ref_index);
        }
        return (0 == count);
    }

    int64_t IOBufAsZeroCopyInputStream::ByteCount() const {
        return _byte_count;
    }

    IOBufAsZeroCopyOutputStream::IOBufAsZeroCopyOutputStream(CordBuffer *buf)
        : _buf(buf), _block_size(0), _cur_block(nullptr), _byte_count(0) {
    }

    IOBufAsZeroCopyOutputStream::IOBufAsZeroCopyOutputStream(
        CordBuffer *buf, uint32_t block_size)
        : _buf(buf)
          , _block_size(block_size)
          , _cur_block(nullptr)
          , _byte_count(0) {
        if (_block_size <= offsetof(CordBuffer::Block, data)) {
            throw std::invalid_argument("block_size is too small");
        }
    }

    IOBufAsZeroCopyOutputStream::~IOBufAsZeroCopyOutputStream() {
        _release_block();
    }

    bool IOBufAsZeroCopyOutputStream::Next(void **data, int *size) {
        if (_cur_block == nullptr || _cur_block->full()) {
            _release_block();
            if (_block_size > 0) {
                _cur_block = iobuf::create_block(_block_size);
            } else {
                _cur_block = iobuf::acquire_tls_block();
            }
            if (_cur_block == nullptr) {
                return false;
            }
        }
        const CordBuffer::BlockRef r = {
            _cur_block->size,
            (uint32_t) _cur_block->left_space(),
            _cur_block
        };
        *data = _cur_block->data + r.offset;
        *size = r.length;
        _cur_block->size = _cur_block->cap;
        _buf->_push_back_ref(r);
        _byte_count += r.length;
        return true;
    }

    void IOBufAsZeroCopyOutputStream::BackUp(int count) {
        while (!_buf->empty()) {
            CordBuffer::BlockRef &r = _buf->_back_ref();
            if (_cur_block) {
                // A ordinary BackUp that should be supported by all ZeroCopyOutputStream
                // _cur_block must match end of the CordBuffer
                if (r.block != _cur_block) {
                    KLOG(FATAL) << "r.block=" << r.block
                           << " does not match _cur_block=" << _cur_block;
                    return;
                }
                if (r.offset + r.length != _cur_block->size) {
                    KLOG(FATAL) << "r.offset(" << r.offset << ") + r.length("
                           << r.length << ") != _cur_block->size("
                           << _cur_block->size << ")";
                    return;
                }
            } else {
                // An extended BackUp which is undefined in regular
                // ZeroCopyOutputStream. The `count' given by user is larger than
                // size of last _cur_block (already released in last iteration).
                if (r.block->ref_count() == 1) {
                    // A special case: the block is only referenced by last
                    // BlockRef of _buf. Safe to allocate more on the block.
                    if (r.offset + r.length != r.block->size) {
                        KLOG(FATAL) << "r.offset(" << r.offset << ") + r.length("
                               << r.length << ") != r.block->size("
                               << r.block->size << ")";
                        return;
                    }
                } else if (r.offset + r.length != r.block->size) {
                    // Last BlockRef does not match end of the block (which is
                    // used by other CordBuffer already). Unsafe to re-reference
                    // the block and allocate more, just pop the bytes.
                    _byte_count -= _buf->pop_back(count);
                    return;
                } // else Last BlockRef matches end of the block. Even if the
                // block is shared by other CordBuffer, it's safe to allocate bytes
                // after block->size.
                _cur_block = r.block;
                _cur_block->inc_ref();
            }
            if (TURBO_LIKELY(r.length > (uint32_t)count)) {
                r.length -= count;
                if (!_buf->_small()) {
                    _buf->_bv.nbytes -= count;
                }
                _cur_block->size -= count;
                _byte_count -= count;
                // Release block for TLS before quiting BackUp() for other
                // code to reuse the block even if this wrapper object is
                // not destructed. Example:
                //    IOBufAsZeroCopyOutputStream wrapper(...);
                //    ParseFromZeroCopyStream(&wrapper, ...); // Calls BackUp
                //    CordBuffer buf;
                //    buf.append("foobar");  // can reuse the TLS block.
                if (_block_size == 0) {
                    iobuf::release_tls_block(_cur_block);
                    _cur_block = nullptr;
                }
                return;
            }
            _cur_block->size -= r.length;
            _byte_count -= r.length;
            count -= r.length;
            _buf->_pop_back_ref();
            _release_block();
            if (count == 0) {
                return;
            }
        }
        KLOG_IF(FATAL, count != 0) << "BackUp an empty CordBuffer";
    }

    int64_t IOBufAsZeroCopyOutputStream::ByteCount() const {
        return _byte_count;
    }

    void IOBufAsZeroCopyOutputStream::_release_block() {
        if (_block_size > 0) {
            if (_cur_block) {
                _cur_block->dec_ref();
            }
        } else {
            iobuf::release_tls_block(_cur_block);
        }
        _cur_block = nullptr;
    }

    IOBufAppender::IOBufAppender()
        : _data(nullptr)
          , _data_end(nullptr)
          , _zc_stream(&_buf) {
    }

    size_t IOBufBytesIterator::append_and_forward(CordBuffer *buf, size_t n) {
        size_t nc = 0;
        while (nc < n && _bytes_left != 0) {
            const CordBuffer::BlockRef &r = _buf->_ref_at(_block_count - 1);
            const size_t block_size = _block_end - _block_begin;
            const size_t to_copy = std::min(block_size, n - nc);
            CordBuffer::BlockRef r2 = {
                (uint32_t) (_block_begin - r.block->data),
                (uint32_t) to_copy, r.block
            };
            buf->_push_back_ref(r2);
            _block_begin += to_copy;
            _bytes_left -= to_copy;
            nc += to_copy;
            if (_block_begin == _block_end) {
                try_next_block();
            }
        }
        return nc;
    }

    bool IOBufBytesIterator::forward_one_block(const void **data, size_t *size) {
        if (_bytes_left == 0) {
            return false;
        }
        const size_t block_size = _block_end - _block_begin;
        *data = _block_begin;
        *size = block_size;
        _bytes_left -= block_size;
        try_next_block();
        return true;
    }
} // namespace butil

void *fermat_memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    return fermat::iobuf::cp(dest, src, n);
} // namespace butil
