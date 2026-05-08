// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <fermat/io/iobuf_base.h>

namespace fermat {

    Allocator<IOBufBase::Block> IOBufBase::alloc;
    IOBufBase::Block * IOBufBase::create_block(size_t nblock) {
        auto *b = alloc.allocate(1);
        auto n = nblock * _block_size;
        auto ptr = static_cast<char*>(mi_aligned_alloc(_alignment, n));
        Allocator<Block>::construct(b, ptr, n);
        return b;
    }

    void IOBufBase::release_block(Block *block) {
        // Use acq_rel to ensure visibility across threads
        if (block->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // 1. First, call the destructor to free 'data'
            if (block->data) {
                mi_free_size_aligned(block->data, block->capacity, _alignment);
                block->data = nullptr;
            }
            // 2. Then, return the memory of the Header (this) to the pool
            alloc.deallocate(block, 1);
        }
    }
}  // namespace fermat
