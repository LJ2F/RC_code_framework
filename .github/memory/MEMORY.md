# 框架维护约定

- 框架面向 STM32H723 + FreeRTOS，当前语言 C11/C++23。
- USB CDC 默认开启，可用 RC_FRAMEWORK_ENABLE_USB 关闭；默认不产生电机或舵机动作。
- 电机模块除用户批准的 6 处 .Maxout→.MaxOut 字段引用外保持原内容；新增协议不包含比赛业务消息和端口绑定。
- USB 单一服务任务拥有发送缓冲；只有完成或确认端点终止才允许复用。
- PMR 保留默认动态内存路径，帧长上限不等于无堆实现。
- 修改后验证 Debug/Release 固件构建及 USB 开/关；接口限制和再生成注意事项见 README.md。

- 可选矩阵为 Algorithm/matrix.hpp，RC_FRAMEWORK_ENABLE_MATRIX 默认关闭；只引入 CMSIS-DSP 矩阵子集。优先使用 try_inverse(out)，不要恢复检查/示例/测试/工具/文档目录。

- PID 与 S 曲线使用 R2 原文件，禁止自行重写、修复算法或追加增强版；后续每项优化、接口调整先征求用户意见。
- PcCom 按原结构最小迁入；业务 OnPacket/ProcessTx 留空，UART 阻塞 write，USB writeAsync，发送 bool 状态回传；不增加回调体系或新编译开关。
