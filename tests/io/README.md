基于当前 `IOBufBase` 和 `Lease` 的 API，以下是为 `iobuf_test.cc` 梳理的完整单元测试用例清单（全部英文注释，无中文）。每个用例都是独立、可执行的，覆盖核心功能、边缘情况和错误处理。

---

### **Lease 基础操作**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `LeaseWriteCommit` | 基本写流程：`borrow()` → `write()` → `commit()` | 数据正确写入，`size()` 匹配 |
| `LeaseWriteWithSize` | 带大小请求的 `borrow(size)`，容量至少为 size | `lease->capacity() >= size`，写入后 commit 成功 |
| `LeaseWriteAcrossSpans` | 写入数据跨越多个块（需 `borrow` 大块） | 所有数据都在 `flatten()` 中正确出现 |
| `LeaseWriteBeyondCapacity` | 写入超过 `remaining()` 返回错误 | `write` 返回 `kOutOfRange`，`size()` 无变化 |
| `LeasePopBack` | 写入后 `pop_back` 撤销部分字节 | `size()` 减少，`flatten()` 结果正确 |
| `LeaseClear` | `clear` 只重置写入光标，不释放容量 | `clear` 后 `size()==0`，可以继续写入新数据（覆盖原位置） |
| `LeaseRemainingAndBool` | `remaining()` 和 `operator bool` 随写入正确更新 | 开始时 `remaining() == capacity()`，写入后减少，写满后 `bool() == false` |

---

### **零拷贝接口 (visit_remaining + advance)**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `VisitRemainingCollectsSpans` | 回调被正确调用，获得每个块的首地址和长度 | 回调次数等于块数，累加容量等于 `capacity()` |
| `AdvanceCommitsWrittenBytes` | 使用 `visit_remaining` 获取指针，外部填充后 `advance(N)` 提交 | `size()` == N，`flatten()` 内容与填充一致 |
| `VisitRemainingStopEarly` | 回调返回 `false` 时停止继续访问后续块 | 回调次数少于总块数 |
| `MixedWriteAndAdvanceNotSupported` | 说明混用会导致未定义行为，不要求测试，但文档需注明 | （可选死亡测试） |

---

### **IOBufBase 写入操作**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `AppendStringView` | `append(std::string_view)` 拷贝数据 | 数据正确，`size()` 累加 |
| `AppendRawData` | `append(void*, size)` 相同 | 同上 |
| `AppendMove` | `append(IOBufBase&&)` 夺取所有权，源被清空 | 目标包含两者数据，源 `size()==0` |
| `AppendToShare` | `append_to` 共享块（对齐兼容） | 目标大小增加，源块引用计数增加且状态变为 `Immutable` |
| `AppendToCopy` | `append_to` 拷贝（对齐不兼容） | 目标数据正确，源数据不变，无共享 |
| `PrependMove` | `prepend(IOBufBase&&)` 移动内容到前端 | 目标的 `flatten()` 结果为 “源+原目标” |
| `PrependToShare` | `prepend_to` 共享到目标前端 | 目标前端为源数据，源块变为 `Immutable` |
| `PrependToCopy` | `prepend_to` 拷贝路径 | 目标前端数据正确，源未变 |

---

### **内存管理与状态机**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `ShrinkRemovesEmptyWritableBlocks` | `shrink()` 删除尾部空的可写块 | 返回释放的字节数，`blocks()` 减少 |
| `ShrinkImmutableSealsLastWritableBlock` | `shrink_immutable()` 将可写块标记为不可变 | 原块变为 `Immutable`，之后 `borrow()` 会分配新块 |
| `CustomConsumesHeadBytes` | `custom(n)` 从头部消费 n 字节 | `size()` 减少 n，被消费的块可能被释放或缩减 |
| `CustomFailsWhenBorrowing` | 有活跃 lease 时 `custom()` 返回错误 | 返回 `kUnavailable` |
| `PopFrontWorksLikeCustom` | `pop_front(n, result)` 与 `custom` 类似且可选复制 | 若 result 非空，则复制数据，消费后 size 减少 |

---

### **错误路径与约束**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `DoubleBorrowFails` | 未 commit 前再次 `borrow()` 返回 `kUnavailable` | 第二次调用返回错误 |
| `CommitWithForeignLeaseDies` | 将不属于此 IOBuf 的 lease 传入 `commit` | 触发 `CHECK` 死亡（`EXPECT_DEATH`） |
| `CommitAfterLeaseClearWorks` | `clear()` 后调用 `commit`（写 0 字节）正常释放租约 | 无崩溃，后续可再次 `borrow` |
| `ShrinkDuringBorrowingFails` | 有活跃 lease 时 `shrink()`/`shrink_immutable()` 返回错误 | 返回 `kUnavailable` |
| `WriteAfterCommitSucceeds` | commit 后可以正常 borrow 并写入新数据 | 第二次写入不影响第一次数据 |

---


### **对齐与分配特性**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `AlignedStringMaintainsAlignment` | `AlignedString<64>` 多次 append 后地址对齐 | `ptr % 64 == 0` |
| `AlignedVectorMaintainsAlignment` | `AlignedVector<double, 128>` 对齐 | `data() % 128 == 0` |
| `IOBufConstructorWithPrefixReserve` | `IOBuf(block_reserve, prefix_reserve)` 预留 Umount 块 | `prepend_blocks() == prefix_reserve`，可无缝前置 |
| `CombineBorrowCreatesContiguousBlock` | `borrow(size, combine_hint > block_size)` 产生单个大块 | `lease->capacity() >= size`，且 `_spans.size() == 1` |

---

### **资源生命周期与线程安全（可选）**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `BlockReferenceCount` | 共享块时引用计数增加，释放时减少 | 通过 `peek` 检查 `block->ref_count`（需 friend）或依赖行为验证 |
| `DestructorReleasesAllBlocks` | IOBuf 析构时所有块被正确回收 | 无内存泄漏（在 ASAN 下运行） |



### **AlignedContainer 集成 (通过 FlattenCustomer)**

| Case | 目的 | 关键断言 |
|------|------|----------|
| `FlattenCustomerToAlignedVector` | 从 IOBuf 移到 `AlignedVector<char>`，`Custom=true` | 目标容器内容正确，源 IOBuf 被完全消费 |
| `FlattenCustomerToAlignedString` | 同上，使用 `AlignedString` | 同上 |
| `FlattenCustomerToStdString` | 标准容器（`std::string`） | 数据正确，源消费正确 |
| `FlattenCustomerCustomFalse` | `Custom=false` 时不调用 `custom()` | 目标有数据，源 IOBuf 大小不变 |
| `FlattenCustomerToFixedBufferEnforcement` | 目标固定容量不足时失败 | 返回 `kOutOfRange`，源数据未改变 |

---