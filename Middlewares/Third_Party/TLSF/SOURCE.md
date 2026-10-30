# TLSF 来源记录

- 上游：https://github.com/mattconte/tlsf
- 固定提交：`deff9ab509341f264addbd3c8ada533678591905`
- 上游版本：TLSF 3.1
- 原作者：Matthew Conte；原始文档来源见上游头文件。
- 选择依据：robocon-library 的 `Third_Party/TLSF` 子模块引用同一提交。
- 许可证：原始 `tlsf.h` 文件头完整保留 BSD 三条款许可与免责声明。
- `tlsf.c`、`tlsf.h`、`README.md` 为上游原文件，没有修改或转换换行。
- 本文件与框架 CMake 接入是本地补充，不覆盖第三方许可。
- 未引入 robocon-library 的 PMR 封装、Async、线程或其他分配器。

| 文件 | 上游 Git blob SHA-1 |
|---|---|
| `tlsf.c` | `af575737c7af62426023dd7e10c7753b4d986f82` |
| `tlsf.h` | `e9b5a91c0bab28bec9a62f6fb3abe5077e6515ec` |
| `README.md` | `982919fc722d869f39413e4757b510cda57fea88` |

本地开关为 `RC_FRAMEWORK_ENABLE_TLSF`，默认关闭。核心本身不提供线程同步。

本地 `.gitattributes` 对上述三个上游文件禁用文本换行转换，以保留原始 Git blob。
