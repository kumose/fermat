## string
| Operation | Length | KString | std::string |
|-----------|--------|---------|--------------|
| Construct | Short | **5.40 ns** | 15.2 ns |
| Construct | Medium | **6.62 ns** | 16.3 ns |
| Construct | Long | **14.4 ns** | 41.3 ns |
| Copy | Short | **6.51 ns** | 15.0 ns |
| Copy | Medium | **12.5 ns** | 16.0 ns |
| Copy | Long | **25.8 ns** | 31.6 ns |
| Move | Short | **6.14 ns** | 16.0 ns |
| Move | Medium | **7.77 ns** | 17.1 ns |
| Move | Long | **14.9 ns** | 43.7 ns |
| Assign | Short | **1.66 ns** | 4.61 ns |
| Append | Short | **2.47 ns** | 3.86 ns |
| Append | Medium | **3.75 ns** | 4.38 ns |
| Append | Long | **11.4 ns** | 11.8 ns |
| Find | Short | 10.8 ns | **8.13 ns** |
| Find | Long | **92.6 ns** | 107 ns |
| Compare Equal | Short | **1.43 ns** | 1.46 ns |
| Compare NotEqual | Medium | 1.29 ns | **1.27 ns** |
| Hash | Short | 3.31 ns | **3.26 ns** |
| Hash | Long | **229 ns** | 232 ns |

## Buffer

### DEBUG
| Operation | Size | std::vector (ns) | fermat::Buffer (ns) |
|-----------|------|--------------|---------------------|
| **Construct** | 4 | 14.1 | **4.73**            |
| | 8 | 14.0 | **4.88**            |
| | 16 | 13.7 | **4.68**            |
| | 32 | 13.7 | **4.99**            |
| | 64 | 14.2 | **5.86**            |
| | 128 | 14.7 | **7.00**            |
| | 256 | 15.7 | **9.00**            |
| | 512 | 38.8 | **13.1**            |
| | 1024 | 50.1 | **22.6**            |
| **PushBack / Append** | 4 | 14.6 | **5.36**            |
| | 8 | 15.5 | **7.52**            |
| | 16 | 17.5 | **11.3**            |
| | 32 | 21.4 | **20.5**            |
| | 64 | 40.7 | **37.4**            |
| | 128 | 64.9 | **65.0**            |
| | 256 | 123 | **126**             |
| | 512 | 261 | **245**             |
| | 1024 | 502 | **487**             |
| | 1000 | 489 | **471**             |
| | 10000 | **4757** | 4879                |
| | 100000 | **54093** | 55697               |
| **Iteration** | 4 | 15.5 | **5.98**            |
| | 8 | 16.9 | **9.42**            |
| | 16 | 18.7 | **12.9**            |
| | 32 | 22.5 | **21.4**            |
| | 64 | **32.2** | 33.1                |
| | 128 | **49.3** | 56.4                |
| | 256 | **90.8** | 115                 |
| | 512 | 180 | **173**             |
| | 1024 | 305 | **349**             |
| | 1000 | **246** | 253                 |
| | 10000 | **2370** | 2370                |
| | 100000 | **23656** | 24077               |
| **RandomAccess** | 4 | **167** | 175                 |
| | 8 | **168** | 174                 |
| | 16 | 167 | **166**             |
| | 32 | 169 | 169                 |
| | 64 | **166** | 170                 |
| | 128 | **169** | 181                 |
| | 256 | 174 | **166**             |
| | 512 | 180 | **168**             |
| | 1024 | 175 | **168**             |
| | 10000 | **1712** | 1755                |
| | 100000 | **1734** | 1753                |
| **InsertMiddle** | 4 | 71.6 | **43.8**            |
| | 8 | 65.8 | **44.8**            |
| | 16 | 55.5 | **53.2**            |
| | 32 | 60.7 | **56.2**            |
| | 64 | 75.8 | **73.5**            |
| | 128 | 97.7 | **72.2**            |
| | 256 | 110 | **90.1**            |
| | 512 | 168 | **117**             |
| | 1024 | **225** | 233                 |
| | 1000 | 1532 | **1513**            |
| | 10000 | 13578 | **12997**           |
| **EraseMiddle** | 4 | 21.0 | **12.8**            |
| | 8 | 29.8 | **23.7**            |
| | 16 | 45.6 | **41.6**            |
| | 32 | **53.3** | 56.0                |
| | 64 | **59.1** | 59.2                |
| | 128 | 69.9 | **81.8**            |
| | 256 | 110 | **95.3**            |
| | 512 | 157 | **129**             |
| | 1024 | 215 | **171**             |
| | 1000 | 1579 | **1405**            |
| | 10000 | **476330** | 482146              |
| **ClearShrink** | 4 | 14.6 | **11.1**            |
| | 8 | 15.7 | **11.9**            |
| | 16 | 15.6 | **11.4**            |
| | 32 | 17.5 | **11.7**            |
| | 64 | 17.3 | **11.5**            |
| | 128 | 18.1 | **14.0**            |
| | 256 | 18.8 | **16.0**            |
| | 512 | 39.9 | **19.8**            |
| | 1024 | 49.0 | **31.2**            |
| | 10000 | **685** | 715                 |
| | 100000 | 7134 | **6930**            |
| **Sort** | 10000 | **345100** | 349422              |
| | 100000 | 4569287 | **4556259**         |

### Release

## Buffer

| Operation | Size | std::vector<int> (ns) | fermat::Buffer<int> (ns) | turbo::InlinedVector<int,128> (ns) | Winner |
|-----------|------|----------------------|--------------------------|-------------------------------------|--------|
| **Construct** | 4 | 14.2 | **4.77** | 1.28 | turbo |
| | 8 | 14.1 | **4.71** | 1.30 | turbo |
| | 16 | 13.6 | **4.71** | 1.29 | turbo |
| | 32 | 13.9 | **4.91** | 1.87 | turbo |
| | 64 | 14.0 | **5.86** | 3.73 | turbo |
| | 128 | 14.5 | **6.97** | 4.66 | turbo |
| | 256 | 15.7 | **8.84** | 16.6 | fermat |
| | 512 | 39.1 | **12.7** | 30.0 | fermat |
| | 1024 | 50.0 | **21.4** | 42.1 | fermat |
| **PushBackSmall** | 4 | 14.5 | **5.61** | – | fermat |
| | 8 | 15.3 | **7.81** | – | fermat |
| | 16 | 17.4 | **14.0** | – | fermat |
| | 32 | 20.7 | **23.5** | – | std |
| | 64 | 39.9 | **39.6** | – | fermat |
| | 128 | 65.0 | **69.3** | – | std |
| | 256 | 143 | **138** | – | fermat |
| | 512 | 286 | **268** | – | fermat |
| | 1024 | 571 | **516** | – | fermat |
| **IterationSmall** | 4 | 15.9 | **5.57** | – | fermat |
| | 8 | 17.9 | **10.1** | – | fermat |
| | 16 | 19.5 | **12.9** | – | fermat |
| | 32 | 26.8 | **34.0** | – | std |
| | 64 | 34.7 | **31.5** | – | fermat |
| | 128 | 54.0 | **47.0** | – | fermat |
| | 256 | 94.9 | **83.8** | – | fermat |
| | 512 | 191 | **152** | – | fermat |
| | 1024 | 340 | **305** | – | fermat |
| **RandomAccessSmall** | 4 | 181 | **183** | – | std |
| | 8 | 186 | **180** | – | fermat |
| | 16 | 189 | **178** | – | fermat |
| | 32 | 179 | **178** | – | std |
| | 64 | 174 | **177** | – | std |
| | 128 | 178 | **174** | – | fermat |
| | 256 | 179 | **183** | – | std |
| | 512 | 177 | **181** | – | std |
| | 1024 | 177 | **178** | – | std |
| **InsertMiddleSmall** | 4 | 73.5 | **42.9** | – | fermat |
| | 8 | 66.3 | **43.9** | – | fermat |
| | 16 | 55.1 | **52.7** | – | fermat |
| | 32 | 59.4 | **57.2** | – | fermat |
| | 64 | 77.3 | **73.2** | – | fermat |
| | 128 | 102 | **73.6** | – | fermat |
| | 256 | 135 | **110** | – | fermat |
| | 512 | 188 | **148** | – | fermat |
| | 1024 | 270 | **258** | – | fermat |
| **EraseMiddleSmall** | 4 | 20.5 | **11.8** | – | fermat |
| | 8 | 28.4 | **22.7** | – | fermat |
| | 16 | 46.1 | **41.4** | – | fermat |
| | 32 | 52.4 | **53.4** | – | std |
| | 64 | 59.7 | **56.0** | – | fermat |
| | 128 | 66.2 | **66.4** | – | std |
| | 256 | 87.2 | **83.6** | – | fermat |
| | 512 | 135 | **107** | – | fermat |
| | 1024 | 180 | **165** | – | fermat |
| **ClearShrinkSmall** | 4 | 14.5 | **10.2** | – | fermat |
| | 8 | 14.5 | **9.83** | – | fermat |
| | 16 | 14.3 | **10.1** | – | fermat |
| | 32 | 14.9 | **10.9** | – | fermat |
| | 64 | 15.1 | **10.7** | – | fermat |
| | 128 | 17.4 | **12.8** | – | fermat |
| | 256 | 18.6 | **14.6** | – | fermat |
| | 512 | 39.4 | **18.3** | – | fermat |
| | 1024 | 48.6 | **31.7** | – | fermat |
| **PushBack** | 1000 | 489 | **469** | – | fermat |
| | 10000 | **4725** | 4711 | – | std |
| | 100000 | **54065** | 55949 | – | std |
| **Iteration** | 1000 | **247** | 248 | – | std |
| | 10000 | 2389 | **2355** | – | fermat |
| | 100000 | 23770 | **23646** | – | fermat |
| **RandomAccess** | 10000 | 1713 | **1710** | – | fermat |
| | 100000 | **1730** | 1758 | – | std |
| **InsertMiddle** | 1000 | 1505 | **1503** | – | fermat |
| | 10000 | 13498 | **13017** | – | fermat |
| **EraseMiddle** | 1000 | 1379 | **1362** | – | fermat |
| | 10000 | **462606** | 463187 | – | std |
| **Sort** | 10000 | 342928 | **342581** | – | fermat |
| | 100000 | 4533225 | **4500183** | – | fermat |
| **ClearAndShrink** | 10000 | **686** | 709 | – | std |
| | 100000 | 6778 | **6694** | – | fermat |

## Bitset

以下是 `fermat::Bitset`、`std::bitset` 和 **`fermat::BitmapView`**（文档/表中简称 **Bitmap**；**不拥有内存的视图**）的性能对比。`BitmapView` 通过 `setup()` 绑定调用方已有的 `uint64_t` 缓冲区，构造/绑定开销极小；表中不适用项标注「—」。

**使用约束（BitmapView）**：仅保证 **`WordType = uint64_t`** 且后备存储 **8 字节对齐**；小于 8 字节的 `WordType` 无法保证性能与安全性。本表数据均来自 `BitmapView<true, uint64_t>`。

| Operation | Bits | fermat::Bitset | std::bitset | fermat::Bitmap (view) |
|-----------|------|----------------|-------------|------------------------|
| **ConstructDefault** | 64 | 0.237 ns | **0.235 ns** | — |
| | 128 | 0.234 ns | **0.232 ns** | — |
| | 256 | 0.239 ns | **0.230 ns** | — |
| | 512 | **0.466 ns** | 0.468 ns | — |
| | 1024 | 0.701 ns | **0.692 ns** | — |
| | 2048 | 7.80 ns | **1.16 ns** | — |
| **ConstructFromULL** | 64 | **0.236 ns** | 0.236 ns | — |
| | 128 | **0.237 ns** | 0.238 ns | — |
| | 256 | **0.347 ns** | 0.350 ns | — |
| | 512 | 1.17 ns | **1.16 ns** | — |
| | 1024 | **7.62 ns** | 7.67 ns | — |
| | 2048 | **7.69 ns** | 7.78 ns | — |
| **SetAll** | 64 | **0.233 ns** | 0.232 ns | 0.578 ns |
| | 128 | 0.234 ns | **0.232 ns** | 0.777 ns |
| | 256 | 0.234 ns | **0.232 ns** | 0.998 ns |
| | 512 | **0.465 ns** | 0.468 ns | 1.67 ns |
| | 1024 | **0.697 ns** | 0.697 ns | 2.98 ns |
| | 2048 | **1.15 ns** | 1.17 ns | 11.2 ns |
| **ResetAll** | 64 | **0.233 ns** | 0.233 ns | 0.941 ns |
| | 128 | **0.234 ns** | 0.254 ns | 1.16 ns |
| | 256 | **0.234 ns** | 0.349 ns | 1.85 ns |
| | 512 | 0.931 ns | **0.922 ns** | 3.82 ns |
| | 1024 | **1.38 ns** | 7.85 ns | 12.2 ns |
| | 2048 | 7.93 ns | **7.87 ns** | 16.1 ns |
| **FlipAll** | 64 | **0.232 ns** | 0.233 ns | 1.71 ns |
| | 128 | **1.49 ns** | 1.50 ns | 1.79 ns |
| | 256 | 1.66 ns | **1.64 ns** | 1.80 ns |
| | 512 | 1.72 ns | **1.63 ns** | 2.48 ns |
| | 1024 | 2.15 ns | **2.13 ns** | 5.29 ns |
| | 2048 | 3.27 ns | **3.19 ns** | 8.56 ns |
| **SingleBitOps** | 64 | 25.9 ns | **24.3 ns** | 204 ns |
| | 128 | **122 ns** | 136 ns | 284 ns |
| | 256 | **244 ns** | 265 ns | 561 ns |
| | 512 | 528 ns | **498 ns** | 1112 ns |
| | 1024 | 1013 ns | **970 ns** | 2174 ns |
| | 2048 | **1931 ns** | 1962 ns | 4608 ns |
| **Count** | 64 | 0.243 ns | **0.233 ns** | 0.468 ns |
| | 128 | **0.238 ns** | 0.240 ns | 0.739 ns |
| | 256 | 0.938 ns | **0.932 ns** | 1.08 ns |
| | 512 | **1.85 ns** | 1.85 ns | 2.11 ns |
| | 1024 | 3.76 ns | **3.71 ns** | 6.25 ns |
| | 2048 | **8.45 ns** | 8.48 ns | 8.62 ns |
| **AnyAllNone** | 64 | 0.233 ns | **0.234 ns** | 1.05 ns |
| | 128 | **0.231 ns** | 0.233 ns | 1.23 ns |
| | 256 | 0.244 ns | **0.234 ns** | 1.85 ns |
| | 512 | 0.246 ns | **0.233 ns** | 2.45 ns |
| | 1024 | 0.233 ns | **0.233 ns** | 4.13 ns |
| | 2048 | 17.3 ns | **0.658 ns** | 17.4 ns |
| **BitwiseOps** | 64 | **0.235 ns** | 0.236 ns | 5.67 ns |
| | 128 | 0.365 ns | **0.350 ns** | 5.36 ns |
| | 256 | 14.9 ns | **14.7 ns** | 6.60 ns |
| | 512 | 30.5 ns | **30.0 ns** | 9.32 ns |
| | 1024 | 45.0 ns | **44.7 ns** | 16.9 ns |
| | 2048 | 23.0 ns | **22.6 ns** | 28.1 ns |
| **ShiftLeft** | 64 | 0.243 ns | **0.239 ns** | 0.726 ns |
| | 128 | 0.496 ns | **0.471 ns** | 1.34 ns |
| | 256 | **1.18 ns** | 1.18 ns | 5.32 ns |
| | 512 | **2.34 ns** | 2.34 ns | 6.88 ns |
| | 1024 | 4.69 ns | **4.65 ns** | 7.13 ns |
| | 2048 | 11.1 ns | **11.1 ns** | 8.27 ns |
| **ShiftRight** | 64 | 0.243 ns | **0.234 ns** | 0.639 ns |
| | 128 | **0.591 ns** | 0.611 ns | 1.79 ns |
| | 256 | **1.24 ns** | 1.25 ns | 2.10 ns |
| | 512 | 7.75 ns | **7.70 ns** | **3.81 ns** |
| | 1024 | 14.4 ns | 14.5 ns | **7.65 ns** |
| | 2048 | 8.38 ns | **8.37 ns** | 15.2 ns |
| **FindFirst** | 64 | **0.235 ns** | 1.31 ns | 0.699 ns |
| | 128 | **0.253 ns** | 2.23 ns | 0.726 ns |
| | 256 | **0.248 ns** | 0.498 ns | 0.700 ns |
| | 512 | **0.247 ns** | 1.12 ns | 0.709 ns |
| | 1024 | **0.234 ns** | 0.478 ns | 0.692 ns |
| | 2048 | **0.367 ns** | 0.492 ns | 0.740 ns |
| **FindNext** | 64 | **0.233 ns** | 0.841 ns | 0.700 ns |
| | 128 | **0.231 ns** | 0.998 ns | 0.724 ns |
| | 256 | 0.377 ns | **0.351 ns** | 0.699 ns |
| | 512 | **0.364 ns** | 0.999 ns | 0.699 ns |
| | 1024 | **0.387 ns** | 1.56 ns | 0.733 ns |
| | 2048 | **0.381 ns** | 1.56 ns | 0.705 ns |

> **Note**:
> - **`fermat::BitmapView`**（表中 Bitmap）为**非拥有型视图**：不分配底层存储，仅包装已有 `uint64_t` 内存；`BM_Bitmap_Setup` 对应 `setup()` 绑定，语义不同于 Bitset 的构造，故未单独列行。
> - **受支持配置**：`BitmapView<true, uint64_t>`，后备区须 **8 字节对齐**；其他 `WordType`（&lt; 8 字节）**不保证**效率与安全性。
> - 表中「—」表示该类型无对应操作或不适用。
> - 粗体为三者中最优。
> - 总体上 `fermat::Bitset` 在构造、set/reset all、find 等占优；`std::bitset` 在部分按位运算与移位略优；`BitmapView` 在部分移位与查询上接近最优（零拷贝绑定），但 `SetAll` 等修改类操作开销更大。按是否已有缓冲区、是否需拥有存储选型。

## Deque

`fermat::Deque<T, Allocator, kDequeSubarraySize>` — third template parameter is elements per internal block (power of 2; default **256** for `int` in `mem_deque_bench`).

| Benchmark | Size | std::deque<int> | fermat::Deque<int> |
|-----------|------|-----------------|---------------------|
| **ConstructEmpty** | – | 26.1 ns | **7.25 ns** |
| **ConstructFill** | 8 | 29.6 ns | **9.29 ns** |
| | 64 | 30.8 ns | **10.8 ns** |
| | 512 | 79.3 ns | **32.5 ns** |
| | 4096 | 826 ns | **203 ns** |
| | 32768 | 10617 ns | **1974 ns** |
| | 100000 | 69040 ns | **6476 ns** |
| **PushBack** | 8 | 28.1 ns | **11.5 ns** |
| | 64 | 63.4 ns | **42.3 ns** |
| | 512 | 535 ns | **249 ns** |
| | 4096 | 3899 ns | **1945 ns** |
| | 32768 | 32319 ns | **17317 ns** |
| | 100000 | 167878 ns | **54955 ns** |
| **PushFront** | 8 | 42.5 ns | **12.1 ns** |
| | 64 | 57.4 ns | **40.5 ns** |
| | 512 | 303 ns | **268 ns** |
| | 4096 | 2464 ns | **2002 ns** |
| | 32768 | 22512 ns | **18141 ns** |
| | 100000 | 67356 ns | **56549 ns** |
| **RandomAccess** | – | 1.63 ns | **1.62 ns** |
| **Iteration** | – | 30287 ns | **29205 ns** |
| **InsertMiddle** | – | 1488 ns | **882 ns** |
| **EraseMiddle** | – | 1767 ns | **767 ns** |
| **Clear** | – | 222 ns | **62.2 ns** |
| **Destruct** | – | 128 ns | **44.5 ns** |


## CordBuffer

| 数据量 | CordBufferBase | std::string | fermat::Buffer | brpc IOBuf | Abseil Cord | Turbo Cord |
|--------|----------------|-------------|----------------|------------|-------------|------------|
| 1 KiB | 17.9 GiB/s | 20.7 GiB/s | **37.3 GiB/s** | 19.8 GiB/s | 12.0 GiB/s | 12.6 GiB/s |
| 10 KiB | **52.7 GiB/s** | 21.2 GiB/s | 46.3 GiB/s | 31.5 GiB/s | 13.2 GiB/s | 12.5 GiB/s |
| 100 KiB | **46.8 GiB/s** | 20.6 GiB/s | 24.8 GiB/s | 29.7 GiB/s | 18.1 GiB/s | 16.4 GiB/s |
| 1 MiB | **44.5 GiB/s** | 12.3 GiB/s | 18.1 GiB/s | 28.3 GiB/s | 18.9 GiB/s | 17.7 GiB/s |
| 10 MiB | **31.1 GiB/s** | 8.01 GiB/s | 8.07 GiB/s | 22.1 GiB/s | 16.3 GiB/s | 15.6 GiB/s |
| 20 MiB | **17.6 GiB/s** | 4.06 GiB/s | 6.34 GiB/s | 13.8 GiB/s | 10.9 GiB/s | 10.9 GiB/s |
| 50 MiB | **12.96 GiB/s** | 2.07 GiB/s | 6.16 GiB/s | 11.1 GiB/s | 8.88 GiB/s | 8.40 GiB/s |
| 100 MiB | **12.15 GiB/s** | 1.52 GiB/s | 6.00 GiB/s | 11.2 GiB/s | 8.61 GiB/s | 8.30 GiB/s |

## List

| Operation | Type | Size | std::list | FermatList |
|-----------|------|------|-----------|-------------|
| **ConstructFill** | int | 8 | 104 ns | **25.6 ns** |
| | | 64 | 908 ns | **194 ns** |
| | | 512 | 7273 ns | **1553 ns** |
| | | 4096 | 58115 ns | **18746 ns** |
| | | 32768 | 445832 ns | **172632 ns** |
| | | 100000 | 1394902 ns | **623287 ns** |
| | Pod3Int | 8 | 101 ns | **29.0 ns** |
| | | 64 | 911 ns | **236 ns** |
| | | 512 | 7236 ns | **1626 ns** |
| | | 4096 | 61271 ns | **19455 ns** |
| | | 32768 | 504719 ns | **175214 ns** |
| | | 100000 | 1522578 ns | **627242 ns** |
| | NonPod | 8 | 126 ns | **40.7 ns** |
| | | 64 | 1137 ns | **273 ns** |
| | | 512 | 8752 ns | **2123 ns** |
| | | 4096 | 68833 ns | **29600 ns** |
| | | 32768 | 569190 ns | **258588 ns** |
| | | 100000 | 2051741 ns | **824019 ns** |
| **PushBack** | int | 8 | 104 ns | **27.3 ns** |
| | | 64 | 906 ns | **209 ns** |
| | | 512 | 7223 ns | **1496 ns** |
| | | 4096 | 56561 ns | **18835 ns** |
| | | 32768 | 455356 ns | **184522 ns** |
| | | 100000 | 1387351 ns | **665841 ns** |
| | Pod3Int | 8 | 108 ns | **29.4 ns** |
| | | 64 | 936 ns | **230 ns** |
| | | 512 | 7513 ns | **1649 ns** |
| | | 4096 | 61505 ns | **18911 ns** |
| | | 32768 | 470429 ns | **181982 ns** |
| | | 100000 | 1451146 ns | **657840 ns** |
| | NonPod | 8 | 315 ns | **214 ns** |
| | | 64 | 2566 ns | **1676 ns** |
| | | 512 | 20273 ns | **13236 ns** |
| | | 4096 | 166146 ns | **118979 ns** |
| | | 32768 | 1366859 ns | **1006582 ns** |
| | | 100000 | 4123495 ns | **3045766 ns** |
| **PushFront** | int | 8 | 106 ns | **36.2 ns** |
| | | 64 | 932 ns | **246 ns** |
| | | 512 | 7246 ns | **1911 ns** |
| | | 4096 | 58713 ns | **19309 ns** |
| | | 32768 | 462043 ns | **175607 ns** |
| | | 100000 | 1398064 ns | **610005 ns** |
| | Pod3Int | 8 | 116 ns | **40.8 ns** |
| | | 64 | 916 ns | **318 ns** |
| | | 512 | 7432 ns | **1862 ns** |
| | | 4096 | 59681 ns | **21554 ns** |
| | | 32768 | 470453 ns | **194726 ns** |
| | | 100000 | 1461504 ns | **655855 ns** |
| | NonPod | 8 | 317 ns | **218 ns** |
| | | 64 | 2547 ns | **1715 ns** |
| | | 512 | 20534 ns | **13335 ns** |
| | | 4096 | 165148 ns | **119366 ns** |
| | | 32768 | 1371924 ns | **993904 ns** |
| | | 100000 | 4218216 ns | **3114959 ns** |
| **Iteration** | int | – | 177757 ns | **119375 ns** |
| | Pod3Int | – | 279754 ns | **119205 ns** |
| | NonPod | – | 690538 ns | **365033 ns** |
| **InsertMiddle** | int | – | 14749 ns | **3502 ns** |
| | Pod3Int | – | 14548 ns | **3588 ns** |
| | NonPod | – | 36818 ns | **21939 ns** |
| **EraseMiddle** | int | – | 15034 ns | **3704 ns** |
| | Pod3Int | – | 15307 ns | **3719 ns** |
| | NonPod | – | 37707 ns | **22772 ns** |
| **Clear** | int | – | 15352 ns | **3099 ns** |
| | Pod3Int | – | 15289 ns | **3261 ns** |
| | NonPod | – | 16072 ns | **4387 ns** |
| **Destruct** | int | – | 14817 ns | **3106 ns** |
| | Pod3Int | – | 14295 ns | **3051 ns** |
| | NonPod | – | 15631 ns | **4154 ns** |
| **Sort** | int | 8 | 380 ns | **62.4 ns** |
| | | 64 | 3650 ns | **450 ns** |
| | | 512 | 37736 ns | **3312 ns** |
| | | 4096 | 373412 ns | **47573 ns** |
| | | 32768 | 3960795 ns | **318230 ns** |
| | | 100000 | 18131280 ns | **1076880 ns** |
| | Pod3Int | 8 | 366 ns | **65.0 ns** |
| | | 64 | 3890 ns | **492 ns** |
| | | 512 | 35804 ns | **3379 ns** |
| | | 4096 | 372562 ns | **47229 ns** |
| | | 32768 | 4559229 ns | **342018 ns** |
| | | 100000 | 19673393 ns | **1150080 ns** |
| | NonPod | 8 | 604 ns | **220 ns** |
| | | 64 | 5963 ns | **1711 ns** |
| | | 512 | 56429 ns | **13966 ns** |
| | | 4096 | 605039 ns | **133718 ns** |
| | | 32768 | 6885749 ns | **1052476 ns** |
| | | 100000 | 26037719 ns | **4283045 ns** |
| **Merge** | int | 8 | 267 ns | **73.0 ns** |
| | | 64 | 2079 ns | **699 ns** |
| | | 512 | 16374 ns | **8703 ns** |
| | | 4096 | 131200 ns | **74066 ns** |
| | | 32768 | 1230810 ns | **641313 ns** |
| | | 100000 | 3705432 ns | **1914264 ns** |
| | Pod3Int | 8 | 258 ns | **77.0 ns** |
| | | 64 | 2034 ns | **651 ns** |
| | | 512 | 16225 ns | **8067 ns** |
| | | 4096 | 132396 ns | **63065 ns** |
| | | 32768 | 1160580 ns | **658471 ns** |
| | | 100000 | 3610203 ns | **1915491 ns** |
| | NonPod | 8 | 362 ns | **190 ns** |
| | | 64 | 3164 ns | **1626 ns** |
| | | 512 | 26998 ns | **16771 ns** |
| | | 4096 | 203199 ns | **154353 ns** |
| | | 32768 | 3417540 ns | **1502295 ns** |
| | | 100000 | 15895164 ns | **6984989 ns** |
| **Splice** | int | 8 | 255 ns | **53.2 ns** |
| | | 64 | 2073 ns | **433 ns** |
| | | 512 | 15705 ns | **5259 ns** |
| | | 4096 | 134097 ns | **63916 ns** |
| | | 32768 | 1640434 ns | **834215 ns** |
| | | 100000 | 6890111 ns | **1925001 ns** |
| | Pod3Int | 8 | 246 ns | **60.9 ns** |
| | | 64 | 1950 ns | **503 ns** |
| | | 512 | 16504 ns | **5388 ns** |
| | | 4096 | 123002 ns | **62905 ns** |
| | | 32768 | 1764178 ns | **502077 ns** |
| | | 100000 | 7605728 ns | **1531865 ns** |
| | NonPod | 8 | 314 ns | **148 ns** |
| | | 64 | 2674 ns | **1223 ns** |
| | | 512 | 21480 ns | **11557 ns** |
| | | 4096 | 183111 ns | **113866 ns** |
| | | 32768 | 3883544 ns | **1138994 ns** |
| | | 100000 | 11066166 ns | **3613814 ns** |
| **PopFrontBack** | int | 8 | 115 ns | **46.0 ns** |
| | | 64 | 979 ns | **280 ns** |
| | | 512 | 7755 ns | **1803 ns** |
| | | 4096 | 63394 ns | **27661 ns** |
| | | 32768 | 520565 ns | **227067 ns** |
| | | 100000 | 2448003 ns | **754838 ns** |
| | Pod3Int | 8 | 117 ns | **39.0 ns** |
| | | 64 | 986 ns | **316 ns** |
| | | 512 | 7822 ns | **1953 ns** |
| | | 4096 | 63639 ns | **26623 ns** |
| | | 32768 | 628337 ns | **263186 ns** |
| | | 100000 | 2692103 ns | **779296 ns** |
| | NonPod | 8 | 276 ns | **179 ns** |
| | | 64 | 2261 ns | **1391 ns** |
| | | 512 | 18066 ns | **10933 ns** |
| | | 4096 | 148427 ns | **98359 ns** |
| | | 32768 | 1394126 ns | **854752 ns** |
| | | 100000 | 4804550 ns | **2685315 ns** |

## LruCache

| Operation | Size | std_map_std_list | std_map_fermat_list | turbo_map_std_list | turbo_map_fermat_list |
|-----------|------|------------------|----------------------|---------------------|------------------------|
| Insert | 1k | 81009 ns | 59168 ns | 42969 ns | **31461 ns** |
| | 10k | 869138 ns | 728951 ns | 514338 ns | **405532 ns** |
| | 100k | 11569495 ns | 8275594 ns | 6816846 ns | **5773787 ns** |
| LookupHigh | 1k | 45.0 ns | 31.0 ns | 28.3 ns | **21.4 ns** |
| | 10k | 51.1 ns | 37.2 ns | 31.8 ns | **25.9 ns** |
| | 100k | 91.7 ns | 45.7 ns | 40.7 ns | **32.9 ns** |
| LookupLow | 1k | 114 ns | 96.8 ns | 85.8 ns | **72.3 ns** |
| | 10k | 126 ns | 107 ns | 106 ns | **90.1 ns** |
| | 100k | 149 ns | 118 ns | 127 ns | **114 ns** |
| Update | 1k | 33.9 ns | 23.6 ns | 22.6 ns | **16.2 ns** |
| | 10k | 42.4 ns | 29.4 ns | 25.6 ns | **21.0 ns** |
| | 100k | 66.5 ns | 38.6 ns | 33.3 ns | **27.8 ns** |
| EraseAll | 1k | 73923 ns | 56230 ns | 42814 ns | **29100 ns** |
| | 10k | 807728 ns | 717045 ns | 485002 ns | **398293 ns** |
| | 100k | 9877764 ns | 8215088 ns | 6418146 ns | **5563505 ns** |
| Touch | 1k | 32.9 ns | 22.6 ns | 21.9 ns | **16.2 ns** |
| | 10k | 39.5 ns | 29.6 ns | 25.2 ns | **21.3 ns** |
| | 100k | 55.6 ns | 38.7 ns | 31.4 ns | **27.9 ns** |

## malloc

| Size (bytes) | malloc | mimalloc | aligned_alloc | posix_memalign | mimalloc_aligned |
|--------------|--------|----------|---------------|----------------|------------------|
| 4 | 14.7 | **3.53** | 52.9 | 66.8 | 10.1 |
| 8 | 14.4 | **3.48** | 53.1 | 65.1 | 10.0 |
| 16 | 14.1 | **3.52** | 53.3 | 67.4 | 10.0 |
| 32 | 14.7 | **3.41** | 52.7 | 63.7 | 8.27 |
| 64 | 15.3 | **3.55** | 52.8 | 50.8 | **3.47** |
| 128 | 39.9 | **13.2** | 62.5 | 67.0 | 12.0 |
| 256 | 39.9 | **13.1** | 63.6 | 70.1 | 12.3 |
| 512 | 39.7 | **16.8** | 76.1 | 85.2 | 15.9 |
| 1024 | 41.9 | **18.5** | 96.7 | 95.6 | 18.1 |
| 2048 | 48.9 | **31.4** | 96.3 | 97.5 | 32.3 |
| 4096 | 51.3 | **39.3** | 98.0 | 99.8 | 40.7 |
| 8192 | 59.3 | **54.3** | 111 | 112 | **53.6** |
| 16384 | 87.2 | **83.3** | 141 | 142 | 85.5 |
| 32768 | 145 | **145** | 198 | 201 | 145 |
| 65536 | 962 | **945** | 1014 | 1015 | 958 |
| 131072 | 1891 | **1879** | 1937 | 1931 | 1891 |
| 262144 | 3706 | 3719 | 3763 | 3772 | **3693** |
| 524288 | 7368 | **7368** | 7472 | 7518 | 7566 |
| 1048576 | 16057 | 16269 | 16069 | **15239** | 16120 |
| 2097152 | 45025 | 45871 | 45290 | 45710 | **45025** |
| 4194304 | 97507 | 97418 | 97527 | **97184** | 97496 |
| 8388608 | 197331 | **195501** | 197853 | 196575 | 197098 |
| 16777216 | 465484 | **460818** | 464363 | 461463 | 468086 |
| 33554432 | (1965) | **1564269** | 9515516 | 9462687 | 1567129 |
| 67108864 | (2009) | **3842050** | 19491213 | 19629266 | 4181661 |
| 134217728 | (2108) | **8150120** | 40359506 | 39760015 | 8138759 |

## pool

| Benchmark | Time (ns) |
|-----------|-----------|
| BM_ObjectPool_SingleThread | **2.02** |
| BM_NewDelete_SingleThread | 11.7 |
| BM_ObjectPool_ConstructDestruct | **1.88** |
| BM_ObjectPool_MultiThread/threads:4 | **2.16** |
| BM_NewDelete_MultiThread/threads:4 | 12.5 |
| BM_ObjectPool_MultiThread/threads:8 | **2.62** |
| BM_NewDelete_MultiThread/threads:8 | 20.2 |
| BM_ObjectPool_MultiThread/threads:16 | **3.77** |
| BM_NewDelete_MultiThread/threads:16 | 29.0 |
| BM_ObjectPool_Batch | **295** |
| BM_NewDelete_Batch | 2789 |


## Priority queue

`pri_queue_bench`: `std::priority_queue<int>` vs `fermat::PriorityQueue<int, 0>`. Push uses `fermat::push_heap` + `fermat::Vector`; Pop uses `std::pop_heap` on both adapters.

> **BoundedPushPop**: at most `Arg` elements, `kBoundedStreamOps` = 2000 push/pop cycles. When `Arg < 2000` the heap stays bounded (mostly push). `BoundedPushPop/2000` ≈ 2000 pushes.

### Push (total ns, n pushes from empty each iteration)

| Size | std::priority_queue | fermat::PriorityQueue | Winner |
|------|---------------------|----------------------|--------|
| 50 | 193 | **80.0** | fermat |
| 100 | 352 | **178** | fermat |
| 200 | 592 | **358** | fermat |
| 500 | 1338 | **917** | fermat |
| 600 | 1555 | **1130** | fermat |
| 700 | 1824 | **1290** | fermat |
| 800 | 2039 | **1452** | fermat |
| 900 | 2355 | **1637** | fermat |
| 1000 | 2369 | **1853** | fermat |
| 2000 | 4461 | **3683** | fermat |
| 10000 | 90493 | **73803** | fermat |
| 100000 | 1253163 | **980509** | fermat |

### Pop (total ns, n pops; heap built in `PauseTiming`)

| Size | std::priority_queue | fermat::PriorityQueue | Winner |
|------|---------------------|----------------------|--------|
| 50 | 425 | **366** | fermat |
| 100 | 772 | **651** | fermat |
| 200 | 1501 | **1297** | fermat |
| 500 | 4183 | **3452** | fermat |
| 600 | 13346 | **4229** | fermat |
| 700 | 19277 | **4961** | fermat |
| 800 | 24393 | **5738** | fermat |
| 900 | 29914 | **6539** | fermat |
| 1000 | 38677 | **12599** | fermat |
| 2000 | 89097 | **70323** | fermat |
| 10000 | 591412 | **541277** | fermat |
| 100000 | 7247497 | **7101736** | fermat |

> std **Pop** / **PushPop** show a sharp slowdown around **n ≈ 600** on this machine; fermat stays smooth until larger n.

### PushPop (total ns, n push then n pop)

| Size | std::priority_queue | fermat::PriorityQueue | Winner |
|------|---------------------|----------------------|--------|
| 50 | 454 | **322** | fermat |
| 100 | 894 | **731** | fermat |
| 200 | 1759 | **1525** | fermat |
| 500 | 4823 | **4242** | fermat |
| 600 | 12652 | **5245** | fermat |
| 700 | 20166 | **6081** | fermat |
| 800 | 27682 | **7176** | fermat |
| 900 | 34164 | **8632** | fermat |
| 1000 | 41378 | **9898** | fermat |
| 2000 | 102329 | **79918** | fermat |
| 10000 | 639528 | **631788** | fermat |
| 100000 | 8193621 | **7817550** | fermat |

### ConstructFromIterators

| Size | std::priority_queue | fermat::PriorityQueue | Winner |
|------|---------------------|----------------------|--------|
| 1000 | 2085 | **1555** | fermat |
| 10000 | 46857 | **20247** | fermat |
| 100000 | 760036 | **670890** | fermat |

### BoundedPushPop (limit = Arg, 2000 stream ops)

| Limit | std::priority_queue | fermat::PriorityQueue | Winner |
|-------|---------------------|----------------------|--------|
| 50 | 19104 | **17647** | fermat |
| 100 | 21771 | **19587** | fermat |
| 200 | 23546 | **21126** | fermat |
| 500 | 32393 | **18893** | fermat |
| 600 | 33473 | **17989** | fermat |
| 700 | 34530 | **16169** | fermat |
| 800 | 36666 | **15653** | fermat |
| 900 | 35737 | **14761** | fermat |
| 1000 | 33822 | **13901** | fermat |
| 2000 | 4451 | **4240** | fermat |

### Other (fermat only)

| Benchmark | Arg | Time (ns) |
|-----------|-----|-----------|
| ChangeRemove | 1000 | 1682 |
| ChangeRemove | 10000 | 22518 |
| ChangeRemove | 100000 | 654188 |
| BoundedPushPop200 | – | 21194 |

## 

```csv
---------------------------------------------------------------------------------------------------
Benchmark                                         Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------------------
BM_AllocFree<ShardedPool>                      14.9 ns         14.9 ns     45889623 items_per_second=67.1175M/s
BM_GetPut<ShardedPool>                         16.5 ns         16.5 ns     41977050 items_per_second=60.5566M/s
BM_MultiThread<ShardedPool>/1/real_time       0.017 us        0.017 us     41037737 items_per_second=58.801M/s
BM_MultiThread<ShardedPool>/2/real_time       0.035 us        0.035 us     19737013 items_per_second=57.9368M/s
BM_MultiThread<ShardedPool>/4/real_time       0.072 us        0.072 us      9093318 items_per_second=55.1898M/s
BM_MultiThread<ShardedPool>/8/real_time       0.179 us        0.179 us      4139232 items_per_second=44.617M/s
BM_MultiThread<ShardedPool>/16/real_time      0.394 us        0.376 us      4341166 items_per_second=40.5964M/s
```


| Benchmark | Time (ns) |
|-----------|-----------|
| BM_AllocFree<ShardedPool> | 14.9 |
| BM_GetPut<ShardedPool> | 16.5 |
| BM_MultiThread<ShardedPool>/1 | 0.017 |
| BM_MultiThread<ShardedPool>/2 | 0.035 |
| BM_MultiThread<ShardedPool>/4 | 0.072 |
| BM_MultiThread<ShardedPool>/8 | 0.179 |
| BM_MultiThread<ShardedPool>/16 | 0.394 |

## sort

| Benchmark | Size | RadixSort (ns) | std::sort (ns) |
|-----------|------|----------------|----------------|
| Random (int) | 1000 | **4199** | 5960 |
| | 10000 | **47702** | 347688 |
| | 100000 | **531912** | 4606486 |
| | 1000000 | **6280935** | 54465458 |
| Sorted (int) | 1000 | 5684 | **3681** |
| | 10000 | 85712 | **50007** |
| | 100000 | 1001991 | **550347** |
| | 1000000 | 12487819 | **7202030** |
| ReverseSorted (int) | 1000 | 5583 | **3235** |
| | 10000 | 85599 | **40810** |
| | 100000 | 1021507 | **457470** |
| | 1000000 | 12160891 | **5487156** |
| Random (uint32) | 1000 | **4219** | 5364 |
| | 10000 | **43790** | 350628 |
| | 100000 | **463296** | 4457025 |
| | 1000000 | **5335934** | 53736546 |
| Sorted (uint32) | 1000 | 5457 | **3196** |
| | 10000 | 52561 | **43136** |
| | 100000 | 807783 | **487833** |
| | 1000000 | 10031521 | **5736180** |
| ReverseSorted (uint32) | 1000 | 5270 | **2403** |
| | 10000 | 51494 | **32475** |
| | 100000 | 813563 | **366872** |
| | 1000000 | 9815373 | **4119764** |
| Random (Person) | 1000 | **23456** | 33536 |
| | 10000 | **233897** | 650246 |
| | 100000 | **2539208** | 8367982 |
| | 1000000 | **48328902** | 112755562 |

## Stack

| 操作 | 数量 | std::stack (ns) | fermat::Stack (ns) |
|------|------|-----------------|---------------------|
| PushPop | 1000 | 919 | **759** |
| | 10000 | 10638 | **9026** |
| | 100000 | 105040 | **92775** |
| Top | 1000 | 0.423 | **0.374** |
| | 10000 | 0.432 | **0.373** |
| | 100000 | 0.426 | **0.372** |
| Emplace | 1000 | 572 | **540** |
| | 10000 | 6669 | **6445** |
| | 100000 | 70315 | **65864** |
| ConstructFromContainer | 1000 | 220 | **20.2** |
| | 10000 | 2869 | **683** |
| | 100000 | 30210 | **6869** |


## stream

> **How to read this table**
> - **RoundTrip / PingPong / Binary\***: Cord chunked I/O vs std on comparable workloads — fair comparisons for Cord use cases.
> - **TokenRead / BulkRead / OStreamFormat (std faster)**: reference **ceiling** for **already-contiguous** data (`std::istringstream` / flat buffer). Shows best achievable throughput on flat memory; **not** a head-to-head disadvantage of `CordInputStringStream` when your data lives in `CordBufferBase` chunks. Pick std for contiguous inputs; pick Cord streams to avoid flattening.

| Operation | Size | std (ns / GiB/s) | fermat (ns / GiB/s) | Winner |
|-----------|------|------------------|---------------------|--------|
| RoundTrip | 100 KiB ~ 10 MiB | 4279047 ns / 1.02 GiB/s | **2277915 ns / 1.97 GiB/s** | fermat |
| PingPong | 1 KiB | 569 ns / 1.68 GiB/s | **442 ns / 2.16 GiB/s** | fermat |
| PingPong | 4 KiB | 1613 ns / 2.37 GiB/s | **1223 ns / 3.12 GiB/s** | fermat |
| PingPong | 16 KiB | 6016 ns / 2.54 GiB/s | **5376 ns / 2.84 GiB/s** | fermat |
| PingPong | 64 KiB | 22454 ns / 2.72 GiB/s | **20319 ns / 3.00 GiB/s** | fermat |
| PingPong | 1 MiB | 456607 ns / 2.14 GiB/s | **363007 ns / 2.69 GiB/s** | fermat |
| PingPong | 4 MiB | 2182219 ns / 1.79 GiB/s | **1530390 ns / 2.55 GiB/s** | fermat |
| TokenRead | 10 MiB | **3041974 ns / 3.21 GiB/s** | 3949660 ns / 2.47 GiB/s | std |
| TokenRead | 20 MiB | **6495905 ns / 3.01 GiB/s** | 8185085 ns / 2.39 GiB/s | std |
| TokenRead | 50 MiB | **16966765 ns / 2.88 GiB/s** | 20384159 ns / 2.40 GiB/s | std |
| BulkRead | 10 MiB | **290643 ns / 33.60 GiB/s** | 560689 ns / 17.42 GiB/s | std |
| BulkRead | 20 MiB | **896455 ns / 21.79 GiB/s** | 1655727 ns / 11.80 GiB/s | std |
| BulkRead | 50 MiB | **3010406 ns / 16.22 GiB/s** | 4961817 ns / 9.84 GiB/s | std |
| BinaryRoundTrip | ~276 KiB | 118439 ns / 2.20 GiB/s | **85280 ns / 3.05 GiB/s** | fermat |
| BinaryWriteOnly | ~276 KiB | 46990 ns / 5.54 GiB/s | **28015 ns / 9.29 GiB/s** | fermat |
| BinaryReadOnly | ~276 KiB | 63575 ns / 4.09 GiB/s | **55996 ns / 4.65 GiB/s** | fermat |
| OStreamFormat | 100 iter | **20362 ns / 1.31 GiB/s** | 144708 ns / 0.188 GiB/s | std |
| OStreamFormat | 1000 iter | **203883 ns / 1.31 GiB/s** | 1451598 ns / 0.188 GiB/s | std |
| OStreamFormat | 10000 iter | **2220670 ns / 1.20 GiB/s** | 14577516 ns / 0.187 GiB/s | std |


## Vector

### DEBUG

| Operation | Size | std::vector (ns) | fermat::Vector (ns) | turbo::InlinedVector<int,128> (ns) | Winner |
|-----------|------|------------------|---------------------|-------------------------------------|--------|
| ConstructSize | 4 | 14.4 | 5.40 | **1.29** | turbo |
| ConstructSize | 8 | 14.2 | 5.47 | **1.29** | turbo |
| ConstructSize | 16 | 13.6 | 5.45 | **1.27** | turbo |
| ConstructSize | 32 | 14.1 | 5.56 | **1.85** | turbo |
| ConstructSize | 64 | 14.1 | 7.19 | **3.73** | turbo |
| ConstructSize | 128 | 15.1 | 8.37 | **4.64** | turbo |
| ConstructSize | 256 | 16.0 | 10.2 | **16.0** | turbo |
| ConstructSize | 512 | 30.6 | 13.9 | **28.4** | fermat |
| ConstructSize | 1024 | 44.2 | 24.6 | **41.7** | fermat |
| PushBackSmall | 4 | 14.3 | **7.16** | – | fermat |
| PushBackSmall | 8 | 15.4 | **8.43** | – | fermat |
| PushBackSmall | 16 | 19.1 | **12.0** | – | fermat |
| PushBackSmall | 32 | 30.5 | **19.3** | – | fermat |
| PushBackSmall | 64 | 40.7 | **34.1** | – | fermat |
| PushBackSmall | 128 | 64.5 | **64.5** | – | tie |
| PushBackSmall | 256 | 123 | 124 | – | **std** |
| PushBackSmall | 512 | 250 | **245** | – | fermat |
| PushBackSmall | 1024 | 491 | **492** | – | std (491 vs 492) |
| EmplaceBackSmall | 4 | 14.5 | **7.63** | – | fermat |
| EmplaceBackSmall | 8 | 15.7 | **8.32** | – | fermat |
| EmplaceBackSmall | 16 | 19.7 | **11.9** | – | fermat |
| EmplaceBackSmall | 32 | 30.2 | **19.3** | – | fermat |
| EmplaceBackSmall | 64 | 40.6 | **38.3** | – | fermat |
| EmplaceBackSmall | 128 | 63.6 | **70.1** | – | std |
| EmplaceBackSmall | 256 | 124 | **131** | – | std |
| EmplaceBackSmall | 512 | 251 | **251** | – | tie |
| EmplaceBackSmall | 1024 | 484 | **486** | – | std |
| IterationSmall | 4 | 15.5 | **6.00** | – | fermat |
| IterationSmall | 8 | 16.7 | **7.99** | – | fermat |
| IterationSmall | 16 | 19.5 | **14.1** | – | fermat |
| IterationSmall | 32 | 27.1 | **31.2** | – | std |
| IterationSmall | 64 | 40.7 | **30.9** | – | fermat |
| IterationSmall | 128 | 55.8 | **46.9** | – | fermat |
| IterationSmall | 256 | 97.2 | **79.8** | – | fermat |
| IterationSmall | 512 | 200 | **142** | – | fermat |
| IterationSmall | 1024 | 319 | **270** | – | fermat |
| RandomAccessSmall | 4 | **165** | 519 | – | std |
| RandomAccessSmall | 8 | **165** | 343 | – | std |
| RandomAccessSmall | 16 | **169** | 258 | – | std |
| RandomAccessSmall | 32 | **166** | 219 | – | std |
| RandomAccessSmall | 64 | **166** | 198 | – | std |
| RandomAccessSmall | 128 | **167** | 189 | – | std |
| RandomAccessSmall | 256 | **165** | 186 | – | std |
| RandomAccessSmall | 512 | **165** | 180 | – | std |
| RandomAccessSmall | 1024 | **166** | 178 | – | std |
| InsertMiddleSmall | 4 | 72.2 | **40.2** | – | fermat |
| InsertMiddleSmall | 8 | 67.1 | **47.1** | – | fermat |
| InsertMiddleSmall | 16 | 54.9 | **53.8** | – | fermat |
| InsertMiddleSmall | 32 | 59.6 | **54.0** | – | fermat |
| InsertMiddleSmall | 64 | 74.9 | **71.2** | – | fermat |
| InsertMiddleSmall | 128 | 75.6 | **70.6** | – | fermat |
| InsertMiddleSmall | 256 | 108 | **87.4** | – | fermat |
| InsertMiddleSmall | 512 | 142 | **114** | – | fermat |
| InsertMiddleSmall | 1024 | 203 | **223** | – | std |
| EraseMiddleSmall | 4 | 19.2 | **12.6** | – | fermat |
| EraseMiddleSmall | 8 | 28.0 | **24.3** | – | fermat |
| EraseMiddleSmall | 16 | 44.9 | **41.6** | – | fermat |
| EraseMiddleSmall | 32 | **53.1** | 52.9 | – | std |
| EraseMiddleSmall | 64 | 61.3 | **56.0** | – | fermat |
| EraseMiddleSmall | 128 | 68.2 | **65.7** | – | fermat |
| EraseMiddleSmall | 256 | 88.2 | **83.1** | – | fermat |
| EraseMiddleSmall | 512 | 121 | **107** | – | fermat |
| EraseMiddleSmall | 1024 | 175 | **155** | – | fermat |
| ClearShrinkSmall | 4 | 14.5 | **11.0** | – | fermat |
| ClearShrinkSmall | 8 | 14.6 | **10.8** | – | fermat |
| ClearShrinkSmall | 16 | 14.3 | **10.8** | – | fermat |
| ClearShrinkSmall | 32 | 15.2 | **11.2** | – | fermat |
| ClearShrinkSmall | 64 | 15.1 | **11.4** | – | fermat |
| ClearShrinkSmall | 128 | 17.3 | **13.3** | – | fermat |
| ClearShrinkSmall | 256 | 18.4 | **14.8** | – | fermat |
| ClearShrinkSmall | 512 | 31.8 | **18.1** | – | fermat |
| ClearShrinkSmall | 1024 | 38.5 | **25.0** | – | fermat |
| PushBack | 1000 | 486 | **471** | – | fermat |
| PushBack | 10000 | 4694 | **4703** | – | std |
| PushBack | 100000 | 53505 | **53696** | – | std |
| EmplaceBack | 1000 | 489 | **474** | – | fermat |
| EmplaceBack | 10000 | 4750 | **4772** | – | std |
| EmplaceBack | 100000 | 54288 | **53643** | – | fermat |
| Iteration | 1000 | 248 | **235** | – | fermat |
| Iteration | 10000 | 2399 | **2395** | – | fermat |
| Iteration | 100000 | 23658 | **23636** | – | fermat |
| RandomAccess | 10000 | **1717** | 1927 | – | std |
| RandomAccess | 100000 | **1793** | 1959 | – | std |
| InsertMiddle | 1000 | **1490** | 1553 | – | std |
| InsertMiddle | 10000 | 13533 | **12977** | – | fermat |
| EraseMiddle | 1000 | 1399 | **1365** | – | fermat |
| EraseMiddle | 10000 | **461903** | 465318 | – | std |
| Sort | 10000 | **342617** | 348570 | – | std |
| Sort | 100000 | 4531808 | **4544515** | – | std |
| ClearAndShrink | 10000 | 692 | **670** | – | fermat |
| ClearAndShrink | 100000 | 6781 | **7017** | – | std |

### Release


| Operation | Size | std::vector<int> (ns) | fermat::Vector<int> (ns) | turbo::InlinedVector<int,128> (ns) | Winner |
|-----------|------|----------------------|--------------------------|-------------------------------------|--------|
| **Construct** | 4 | 14.6 | **5.31** | 1.28 | turbo |
| | 8 | 14.4 | **5.36** | 1.30 | turbo |
| | 16 | 13.8 | **5.49** | 1.29 | turbo |
| | 32 | 13.9 | **5.75** | 1.87 | turbo |
| | 64 | 14.4 | **7.20** | 3.73 | turbo |
| | 128 | 15.0 | **8.41** | 4.66 | turbo |
| | 256 | 16.0 | **10.3** | 16.6 | fermat |
| | 512 | 30.8 | **14.0** | 30.0 | fermat |
| | 1024 | 43.5 | **23.9** | 42.1 | fermat |
| **PushBackSmall** | 4 | 14.5 | **6.93** | – | fermat |
| | 8 | 15.4 | **8.99** | – | fermat |
| | 16 | 17.2 | **12.5** | – | fermat |
| | 32 | 20.5 | **21.4** | – | std |
| | 64 | 38.9 | **40.1** | – | std |
| | 128 | 64.1 | **64.8** | – | std |
| | 256 | 123 | **123** | – | tie |
| | 512 | 250 | **244** | – | fermat |
| | 1024 | 488 | **481** | – | fermat |
| **EmplaceBackSmall** | 4 | 15.0 | **6.66** | – | fermat |
| | 8 | 15.9 | **7.92** | – | fermat |
| | 16 | 17.7 | **11.8** | – | fermat |
| | 32 | 22.1 | **19.3** | – | fermat |
| | 64 | 39.7 | **33.9** | – | fermat |
| | 128 | 64.9 | **64.0** | – | fermat |
| | 256 | 123 | **123** | – | tie |
| | 512 | 250 | **242** | – | fermat |
| | 1024 | 489 | **489** | – | tie |
| **IterationSmall** | 4 | 15.4 | **5.70** | – | fermat |
| | 8 | 19.0 | **8.11** | – | fermat |
| | 16 | 18.5 | **12.5** | – | fermat |
| | 32 | 23.5 | **21.5** | – | fermat |
| | 64 | 32.0 | **25.9** | – | fermat |
| | 128 | 52.4 | **46.4** | – | fermat |
| | 256 | 96.8 | **78.7** | – | fermat |
| | 512 | 170 | **145** | – | fermat |
| | 1024 | 301 | **273** | – | fermat |
| **RandomAccessSmall** | 4 | 174 | **174** | – | tie |
| | 8 | 180 | **175** | – | fermat |
| | 16 | 174 | **175** | – | tie (std 174, fermat 175) |
| | 32 | 180 | **172** | – | fermat |
| | 64 | 175 | **175** | – | tie |
| | 128 | **177** | 178 | – | std |
| | 256 | 173 | **173** | – | tie |
| | 512 | **173** | 177 | – | std |
| | 1024 | **172** | 180 | – | std |
| **InsertMiddleSmall** | 4 | 71.1 | **40.1** | – | fermat |
| | 8 | 64.5 | **47.2** | – | fermat |
| | 16 | 55.5 | **55.3** | – | fermat |
| | 32 | 58.9 | **56.0** | – | fermat |
| | 64 | 81.3 | **74.5** | – | fermat |
| | 128 | 85.5 | **77.5** | – | fermat |
| | 256 | 122 | **106** | – | fermat |
| | 512 | 172 | **146** | – | fermat |
| | 1024 | **261** | 262 | – | std |
| **EraseMiddleSmall** | 4 | 20.5 | **12.0** | – | fermat |
| | 8 | 30.2 | **22.6** | – | fermat |
| | 16 | 48.0 | **41.2** | – | fermat |
| | 32 | **53.0** | 53.2 | – | std |
| | 64 | 63.3 | **55.6** | – | fermat |
| | 128 | 68.6 | **66.0** | – | fermat |
| | 256 | 113 | **104** | – | fermat |
| | 512 | 145 | **135** | – | fermat |
| | 1024 | 218 | **202** | – | fermat |
| **ClearShrinkSmall** | 4 | 14.6 | **10.6** | – | fermat |
| | 8 | 14.6 | **10.1** | – | fermat |
| | 16 | 14.6 | **10.1** | – | fermat |
| | 32 | 15.6 | **10.7** | – | fermat |
| | 64 | 15.5 | **10.5** | – | fermat |
| | 128 | 17.8 | **13.1** | – | fermat |
| | 256 | 18.8 | **14.4** | – | fermat |
| | 512 | 33.0 | **18.5** | – | fermat |
| | 1024 | 43.0 | **25.5** | – | fermat |
| **PushBack** | 1000 | 487 | **478** | – | fermat |
| | 10000 | **4693** | 4734 | – | std |
| | 100000 | **53855** | 54134 | – | std |
| **EmplaceBack** | 1000 | 482 | **467** | – | fermat |
| | 10000 | 4778 | **4706** | – | fermat |
| | 100000 | 54018 | **53590** | – | fermat |
| **Iteration** | 1000 | **236** | 238 | – | std |
| | 10000 | 2385 | **2361** | – | fermat |
| | 100000 | 23576 | **23511** | – | fermat |
| **RandomAccess** | 10000 | 1739 | **1698** | – | fermat |
| | 100000 | 1732 | **1725** | – | fermat |
| **InsertMiddle** | 1000 | **1493** | 1517 | – | std |
| | 10000 | 13698 | **12917** | – | fermat |
| **EraseMiddle** | 1000 | 1396 | **1367** | – | fermat |
| | 10000 | **457981** | 462684 | – | std |
| **Sort** | 10000 | **339544** | 343151 | – | std |
| | 100000 | 4568286 | **4549312** | – | fermat |
| **ClearAndShrink** | 10000 | 690 | **667** | – | fermat |
| | 100000 | 6713 | **6667** | – | fermat |