You're right, the previous list missed several important test points. Below is a **comprehensive** unit test plan covering all aspects of the latest `CordBufferBase` code, including edge cases and recently added/modified methods.

```markdown
# Comprehensive Unit Test Points for `CordBufferBase`

## 1. `BufferRef` – Core Building Block

### 1.1 Factory Methods
- `create_write_able(n)`
  - Buffer allocated with exact size `n`, `size() == 0`, `capacity() == n`, `write_able() == n`.
  - `buffer.use_count() == 1`.
- `setup_write_able(shared_ptr&&)` / `setup_write_able(shared_ptr&&, off, len)`
  - Transfers ownership, asserts `use_count == 1`.
  - Range correctly set (offset/length).
- `reference(shared_ptr&, off, len)`
  - Shares ownership, does not modify original buffer.
  - `size() == len`, `offset() == off`.
- `assign(shared_ptr&&, off, len)`
  - Moves ownership, sets range, `use_count == 1`.

### 1.2 Write Operations
- `append(data, size)`
  - Writes up to `write_able()` bytes, returns actual written.
  - `range.length` increases accordingly.
  - Cannot write beyond `capacity()`.
- `borrow(void**, int*)` – **note signature changed to `int*` (not `int64_t*`)**
  - Returns `false` when `use_count > 1` or no writable space.
  - Otherwise returns pointer to writable region, sets `*size` to available bytes, and **automatically extends `range.length` by that amount**.
  - After borrow, `size()` equals old length + borrowed size.
- `backup(size_t size)`
  - Only allowed when `use_count == 1` (crashes otherwise).
  - Reduces `range.length` by at most `size`, returns actual reduction.
  - When `size > range.length`, clears the segment.

### 1.3 Pop Operations
- `pop_front(size_t n)`
  - Removes from front, adjusts `offset` and `length`.
  - Returns actual number of bytes removed (may be less than `n` if `length < n`).
- `pop_back(size_t n)`
  - Removes from tail, only reduces `length`, returns actual removed.

### 1.4 Introspection
- `write_able()`: 0 if `use_count > 1` or no space, else remaining bytes.
- `capacity()`: total buffer size minus `offset`.
- `is_unique()`: `use_count == 1`.

## 2. `CordBufferBase` – Construction & State

### 2.1 Default and Move Construction
- Default: `size() == 0`, `blocks() == 0`, `_write_buffer` points to `kZeroBuffer`.
- Move constructor (current implementation):
  ```cpp
  CordBufferBase(CordBufferBase &&rhs) noexcept
      : _views(std::move(rhs._views)),
        _total_size(rhs._total_size),
        _write_buffer(_views.empty() ? const_cast<...>(&kZeroBuffer) : &_views.back())
  ```
- Source’s `_views` and `_total_size` are cleared.
- Source’s `_write_buffer` reset to `kZeroBuffer`.
- Target’s `_write_buffer` points to the last segment if non‑empty, else sentinel.
- Move assignment: similar, also clears `rhs._views`.

### 2.2 Copy/Move Assignment
- Copy assignment: `operator=(const CordBufferBase&)` uses `share()` + swap → shallow copy (shared buffers).
- Move assignment: transfers ownership, source cleared, `rhs._views` also cleared (code explicitly calls `rhs._views.clear()`).

## 3. Adding Data – Ownership Semantics

### 3.1 `append_reference` (shared, read‑only)
- Single `BufferRef` (lvalue/rvalue): adds reference, **resets `_write_buffer` to `kZeroBuffer`**, updates `_total_size`.
- Vector overloads: batch add, order preserved, `_write_buffer` reset.

### 3.2 `prepend_reference`
- Inserts at front, order preserved (reverse iteration on source).
- Does **not** change `_write_buffer` (unless cord was empty and new tail is writable? – current code does not set it, so `_write_buffer` stays as before).

### 3.3 `append_writeable` (exclusive, writable)
- Requires `ref.is_unique()` (calls `DKCHECK`), non‑zero size.
- After appending, `_write_buffer` points to the new tail (which becomes writable).
- Vector version:
    - Iterates through `refs`, stops at first zero‑size element (`break`).
    - Accumulates `added` only for successfully added buffers.
    - Only updates `_write_buffer` if at least one buffer was added.
    - **Important**: zero‑size elements cause early exit, discarding later non‑zero buffers. This is intentional but must be tested.

### 3.4 `append_buffer` (via `shared_ptr`)
- `append_buffer(const shared_ptr&, offset, length)`: creates reference, resets `_write_buffer`.
- `append_buffer(shared_ptr&&, offset, length)`: moves ownership, resets `_write_buffer`.
- No separate `append_buffer` without offset/length (only the offset/length versions exist in this latest code).

### 3.5 `prepend_buffer`
- Similar to `prepend_reference` but with `shared_ptr` input.
- Const version **may** update `_write_buffer` if the cord’s tail becomes unique and writable (after prepend, the original tail remains the same; check logic: `if (_views.back().is_unique()) _write_buffer = &_views.back();`).
- Rvalue version does not update `_write_buffer`.

## 4. Batch Creation Helpers

### 4.1 `create_buffers(size_t bytes_needed)`
- Returns `std::vector<BufferRef<Alignment>>` (changed from returning `shared_ptr<Buffer>` in previous versions).
- Each block is `BlockSize` except possibly the last (smaller if needed).
- Total capacity ≥ `bytes_needed`.

### 4.2 `create_big_buffer(size_t bytes_needed)`
- Returns a single `BufferRef` with size `BlockSize * ceil(bytes_needed/BlockSize)`.
- Useful for merging many small blocks into one large buffer.

## 5. Writing Through `output_next()` / `output_backup()`

### 5.1 `output_next()` – modern span interface
- Returns `turbo::span<char>` of writable area.
- Internally calls `borrow` with `int*` (not `size_t*`). The returned span length is `available` cast to `size_t`.
- If no writable space, allocates a new block (`get_write_able_buffer` creates one of size `BlockSize`).
- After call, the span’s entire length is considered written (i.e., `range.length` is increased by that amount).

### 5.2 `output_next(void**, int*)` – traditional interface
- Same semantics, but returns `bool` and writes `*size` as `int`.

### 5.3 `output_backup(int n)`
- Rolls back last `n` bytes (must be ≤ `_total_size`). If `n >= _total_size`, clears the cord by swapping with an empty one.
- Works across multiple buffers, empties zero‑length segments.
- After rollback, updates `_write_buffer`:
    - If any segments left, `_write_buffer = &_views.back()`.
    - Else `_write_buffer = &kZeroBuffer`.

## 6. Removing Data

### 6.1 `pop_front(size_t n)`
- Removes from front, may consume full segments.
- Uses `BufferRef::pop_front` on the front segment repeatedly.
- Stops when `n` bytes removed or cord empty.
- Does **not** update `_write_buffer` (unless cord becomes empty, which would call `clear()` indirectly? Actually `clear()` is called when `n >= _total_size`).

### 6.2 `pop_front_buffer()`
- Removes the entire front segment, updates `_total_size` and `_views`.
- If cord becomes empty, resets `_write_buffer` to `kZeroBuffer`.

### 6.3 `pop_back(size_t n)`
- Removes from tail, may consume full segments.
- Uses direct modification of `back.range.length`.
- After removal, updates `_write_buffer`:
    - If `_views.empty()` or new tail is **not** writable, `_write_buffer = &kZeroBuffer`.
    - Else `_write_buffer = &_views.back()`.
- Important: the new tail might have been the same as before if only partially trimmed, but writability may have changed? The check `!_views.back().write_able()` handles that.

### 6.4 `pop_back_buffer()`
- Removes the whole tail segment, similar `_write_buffer` update logic.

## 7. Appending/Prepending Entire Cords

### 7.1 `append(const CordBufferBase& rhs)` – shallow copy
- Copies each `BufferRef` (shared ownership).
- Resets `_write_buffer` to `kZeroBuffer`.

### 7.2 `append(CordBufferBase &&rhs)` – move
- Moves all segments, clears `rhs`.
- Updates `_write_buffer`:
    - If new tail is writable, `_write_buffer = &_views.back()`.
    - Else `_write_buffer = &kZeroBuffer`.

### 7.3 `prepend(const CordBufferBase& rhs)` – shallow copy
- Inserts copies at front (reverse order to preserve original order).
- Does **not** change `_write_buffer`.

### 7.4 `prepend(CordBufferBase &&rhs)` – move
- Moves segments to front (reverse iteration).
- If original cord was empty and the new tail (which was the last segment of `rhs` before move) is writable, sets `_write_buffer` to that segment.

## 8. Sharing and Copying

### 8.1 `share()`
- Creates a new cord with the same `BufferRef` instances (shared ownership).
- `_write_buffer` reset to `kZeroBuffer` to prevent writes.

### 8.2 `copy()`
- Deep copy: appends data byte by byte (using `append(const void*, size_t)`).
- Result has independent buffers and `_write_buffer` pointing to the last writable segment.

## 9. Iterators (const)

### 9.1 `CordIterator` behavior
- Respects `range.offset` and `range.length` (constructor and `operator++` use `_cur->range.length`).
- `operator*()` returns `char` (by value).
- Pre/post increment, equality comparison.
- Edge: when `_index_read >= _cord->size()`, `operator++` returns immediately (no increment).
- Test across multiple segments with varying ranges.

## 10. `InputStream` (ZeroCopyInputStream adapter)

### 10.1 `next()`
- Returns chunks from each segment’s effective range (`range.offset` + `range.length`).
- Skips empty segments.
- Stores chunk size in `_last_chunk_size`.

### 10.2 `back_up(int count)`
- Supports **multiple** back‑ups on the same chunk because it does `_last_chunk_size -= count` (instead of setting to 0).
- Moves `_index` back if necessary (when count goes into previous segment).
- Validates `count <= _last_chunk_size`.

### 10.3 `skip(int count)`
- Can cross segment boundaries.
- Updates `_index`, `_offset`, `_byte_count`.
- Resets `_last_chunk_size = 0` after skip.

### 10.4 `byte_count()` and `remain()`
- Return total consumed bytes and remaining bytes.

### 10.5 Internal `_skip_empty()`
- Advances `_index` past segments with zero `range.length`.

## 11. High‑Performance Read Methods

### 11.1 `append_by_readv(IOReader&, max_limited, restart_block)`

**Preconditions and edge cases:**
- When `max_limited == 0`, returns 0 immediately.
- `restart_block == false`:
    - First calls `output_next()` to get current tail span and adds it to `vecs`.
    - If still need more capacity (`collect < max_limited`), computes number of additional full blocks (`n`) and creates them.
    - For `n-1` full blocks, adds spans using `ref.capacity()`.
    - For the last block, computes `last = min(max_limited - collect, ref.capacity())` and adds a partial span.
- `restart_block == true`:
    - Skips using the existing tail, directly allocates `n` new buffers (using `kMaxReadVSpans` limit).
- After `readv`:
    - On error, rolls back if `!restart_block` (rolls back only the first span that came from `output_next`).
    - On success, splits the read bytes among the buffers:
        - The first span (if `!restart_block`) is handled separately: if `rsize <= span_size`, rolls back the unused part and returns.
        - Otherwise, iterates over `buffers` and sets `it->range.length = min(remaining, it->capacity())` (note: uses `capacity()`, not `size()`, because initially `size() == 0`).
    - Finally calls `append_writeable(std::move(buffers))` to add all newly allocated buffers.

**Test points:**
- `restart_block = false` with enough space in existing tail.
- `restart_block = false` where tail capacity is insufficient, forcing new buffers.
- `restart_block = true` with various `max_limited`.
- Partial read (rsize < total capacity) – ensures trailing bytes are not appended.
- Read error – ensures rollback happens correctly.
- Zero‑byte read (rsize == 0) – should not add any buffers.
- When `collect` already reaches `max_limited` before allocating new buffers – then `buffers` remains empty, and the later loop is skipped (safe because `rsize <= span_size` will hold and early return).

### 11.2 `append_by_read(IOReader&, max_limited)`
- Loop: each iteration calls `output_next()`, reads up to the span size, updates `total_read`, and on partial read rolls back unused part and breaks.
- Test partial read, full read, read failure.

### 11.3 `append_by_pread(IOReader&, offset, max_limited)`
- Similar to `append_by_read` but with `pread` and offset.
- Test that `cur_offset` increments correctly across blocks.

## 12. Clear and Swap

### 12.1 `clear()`
- Clears `_views`, sets `_total_size = 0`, `_write_buffer = &kZeroBuffer`.

### 12.2 `swap(CordBufferBase& other)`
- Swaps `_views`, `_total_size`, `_write_buffer`.

## 13. Operators

### 13.1 `operator<<` for arithmetic types
- Calls `cat(v)` which uses `turbo::str_cat` and appends.
- Verify that numbers are formatted as strings.

### 13.2 `operator<<` for string/span/vector/Buffer
- Append the data (deep copy).

### 13.3 Assignment from string/span/vector/Buffer
- Clears first, then appends.

## 14. Receivers / Appenders (Compatibility)

### 14.1 `ContainerAppender<CordBufferBase>`
- `append(const char*, size_t)` forwards to `cord.append()`.
- `clear()`, `resize()`, `reserve()` (resize/reserve are no‑ops).
- `is_dynamic()`, `size()`, `capacity()`.

### 14.2 `ContainerReceiver<CordBufferBase>`
- Similar, but owns the cord.
- `release()` moves out the cord.

## 15. Type Traits

### 15.1 `is_cord_buffer<T>`
- Specialized for `CordBufferBase`, with `alignment` and `block_size` static members.
- `is_cord_buffer_v` helper.

## 16. Edge Cases & Stress Tests

### 16.1 Empty Cord
- All operations that read (begin, end, input_stream, front_buffer, back_buffer) should crash with `KCHECK` if empty? Actually `front_buffer()` and `back_buffer()` have `KCHECK(!_views.empty())`. Unit tests should verify that they abort (or use death tests).
- `pop_front(0)` and `pop_back(0)` do nothing.
- `pop_front(n)` when `n >= _total_size` calls `clear()`.

### 16.2 Single‑Segment Cord
- Write, read, pop, backup.

### 16.3 Multi‑Segment Cord
- Random interleaving of append/prepend/pop operations.
- Verify `_total_size` consistency.

### 16.4 Shared Cord (COW)
- Create cord A, fill with data.
- Share to cord B via `share()`.
- Append to A – should allocate new buffers, not affect B’s data.
- Append to B – similar.
- Verify that `write_able()` returns 0 for segments that are shared.

### 16.5 Very Large Data
- Append > 2GB (if `size_t` supports) – test for overflow or performance.
- `create_buffers` with huge `bytes_needed` – may throw `bad_alloc`.

### 16.6 Error Paths
- `append_buffer` with out‑of‑range offset/length returns `invalid_argument_error`.
- `append_by_readv` with invalid `reader` that returns error.
- `output_backup` with `n > _total_size` – clears cord.
- `borrow` on shared buffer returns `false`.

## 17. Concurrency (optional, stress test)
- Multiple threads appending to the same cord (with external synchronization) – not thread‑safe by design, but use of `use_count` checks prevents corrupting shared buffers.
- Sharing a cord between threads and reading while one thread writes – should not crash (data races may still occur, but COW helps).

## 18. Memory Leak Checks
- All `shared_ptr` references should be released when cords go out of scope.
- `BufferRef` with `assign` transferred ownership, no double free.
- `create_buffers` and `create_big_buffer` return buffers that must be properly consumed.
```

This list should cover **all** methods and critical edge cases present in the latest code. If anything specific is still missing, please point it out.