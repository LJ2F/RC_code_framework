# TLSF 来源与本地修改

- 上游：https://github.com/mattconte/tlsf
- 基线提交：`deff9ab509341f264addbd3c8ada533678591905`，TLSF 3.1。
- 原作者：Matthew Conte。完整 BSD 三条款许可与免责声明保留在 tlsf.h。
- 选择依据：robocon-library 的 Third_Party/TLSF 子模块引用同一提交。
- 未引入 robocon-library 的 PMR 封装、Async、线程或其他分配器。

当前 tlsf.c / tlsf.h 含本地补丁，不能再把当前文件视作未修改上游。
上游 README.md 保留原文。以下 blob 标识的是引入时的原版，便于对照差异：

| 原版文件 | 上游 Git blob SHA-1 |
|---|---|
| tlsf.c | af575737c7af62426023dd7e10c7753b4d986f82 |
| tlsf.h | e9b5a91c0bab28bec9a62f6fb3abe5077e6515ec |
| README.md | 982919fc722d869f39413e4757b510cda57fea88 |

## 本地补丁（2026-10-30）

1. 新增 tlsf_config.h：TLSF_MAX_POOL_SIZE 默认 65536，单位为每段传入区域的总字节数；
   0 保留上游架构相关索引范围。非零支持 1024..1073741824。
2. tlsf.h 仅增加配置头引用，原接口、作者和许可保留。
3. tlsf.c 根据最大区域容量向上取整计算 FL_INDEX_MAX，减少空闲链表指针表。
   保留 SL_INDEX_COUNT_LOG2=5，未引入 LVGL、RT-Thread 或 tlsf-bsd 代码。
4. tlsf_add_pool 在减去管理开销前拒绝空句柄、空地址、容量不足和超出配置上限。
   块容量等于 block_size_max 时拒绝，避免一级索引越界。
5. 核心不增加线程锁、不跟踪区域登记。多区域登记、重叠检查、总量统计及可选同步
   位于 Algorithm/memory/tlsf_pool.hpp/.cpp，不使用上游堆建立登记表。

构建定义由 RC_FRAMEWORK_TLSF_MAX_POOL_SIZE 通过 PUBLIC TLSF_MAX_POOL_SIZE 传播；
独立编译核心时可用 -DTLSF_MAX_POOL_SIZE=...。所有核心源码使用同一配置，修改后全量重编译。
.gitattributes 对三个原有文件保留 -text；它用于禁止 Git 自动换行转换，不代表文件未修改。

## 范围

- RC_FRAMEWORK_ENABLE_TLSF 默认 OFF；开启不自动创建池、预留 RAM、绑定外设或启动任务。
- 调用者保证每段实际可写、对齐、生命周期有效；原始 C 接口仍要求可信缓冲区与长度。
- 配置按单段限制，多个区域容量总和可以更大；一次分配仍只来自一个物理连续区域。
- CPU 与 DMA 属性不由 TLSF 推断；不改变原有 DMA 缓冲、FreeRTOS 堆或全局 new/delete。