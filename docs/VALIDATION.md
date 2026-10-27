# 验证记录

日期：2026-10-27。测试对象为本次 C++23 与通信/外设升级后的框架。
未连接实物，不把模拟结果当作硬件验证。

## 自动验证

环境：Windows、CMake 4.2.3、Ninja 1.13.2，Arm GNU Toolchain 15.2.Rel1 /
GCC 15.2.1；主机 MinGW-w64 GCC/G++ 15.2.0。固件 C11/C++23，均关闭方言扩展，
C++ 保持无 RTTI、无异常、无线程安全静态初始化。已有 Cube 工具版本设置保持原样，
没有虚构 GCC 15.2.1 对应的 Cube 工具包版本。

使用 tools/validate.py，在 build/validation 下独立目录执行全新配置和清理构建，
每组均包括默认固件、全部公共头文件（包含 C 桥接头）和适用的示例编译。
USB OFF 的编译数据库不含 USB 中间件、UsbPort.cpp 或 USB 示例。

| 配置 | 结果 | FLASH 字节 | DTCMRAM 字节 |
|---|---|---:|---:|
| Debug-usb-on | 通过 | 97600 | 52880 |
| Debug-usb-off | 通过 | 66632 | 41384 |
| Release-usb-on | 通过 | 44060 | 52448 |
| Release-usb-off | 通过 | 31068 | 40952 |

上表为默认固件链接器内存占用，DTCMRAM 包括链接预留堆栈段；
RAM_D1/D2/D3、ITCM 默认均为 0。示例默认不链接，显式接入后内存占用会增加。

主机 Debug 和 Release 各通过 4 个测试套件，检查使用主动失败断言，不依赖 Release 中会被关闭的 assert：

| 套件 | 已覆盖 |
|---|---|
| remote_protocol | 原遥控 CRC8、有效帧、分块输入和超时，继续保留原测试 |
| packet_protocol | 独立逐位 CRC 参考、R2 原格式往返、空/最大/超长载荷、全部 513 个分块位置、连包、噪声、长度/CRC/尾部错误恢复、内嵌有效帧恢复、单遍输入迭代器、发送失败回传、适配回调、半包清理、固定存储包装器复制/移动 |
| usb_backend | 真实 UsbPort.cpp、CDC 适配 C 文件和 ST CDC 类编译；未配置/空类/非法 classId/挂起/非法请求/ISR 拒绝、8 项完整入队与队列满、复制后的生命周期、忙碌重试、完成、挂起保留缓冲、恢复与复位旧代次过滤、错误统计、任务创建失败 |
| peripherals | 真实 UartPort.cpp 复制路径、Logger 截断/容量/忙碌/过滤、舵机端点/限幅/非法配置/启动失败；Xbox 兼容/校验回调/ID/精确长度/重同步/掉线；红外半包/连包/快速 ACK/错误 ACK/重入拒绝/100 Hz tick 换算与回绕超时；HWT101 校验与正负角度/有效帧和角度帧计数 |

USB 测试模拟 RTOS 和 LL 硬件提交事件，未执行的硬件控制接口会主动终止测试，
避免用无条件成功的空实现掩盖调用范围。发送失败测试对 LL 的“已终止端点”约定进行模拟；
真实寄存器和端点关闭时序必须上板验证。未使用外部模型检查器穷举所有任务交错，
跨任务/ISR 的正确性证据为代码所有权审查、原子计数、临界区和上述事件序列测试。

## 内存与默认行为

协议连续接收 10000 次最大帧后，PMR 未释放字节数回到循环前值；
测试结束所有 PMR 分配均释放，本机峰值 3376 字节（包括测试中并存的多个包和管理器）。
这是主机测试峰值，不是 MCU 的堆预算。接收候选帧不超过 512 字节。
C++ newlib 堆与 FreeRTOS 15360 字节堆分开，不改写全局分配器；
无异常下 OOM 仍终止。静态 USB 对象本机 sizeof 为 7392 字节，ARM 符号尺寸见 map。

Debug 的编译栈报告：writeAsync 自身 576 字节、ServiceOnce 自身 96 字节、
OnRxFromIsr 自身 304 字节。它们不含完整调用链；USB 服务栈为 1024 字节，
应用调用任务建议至少从 2 KiB 开始，需上板测高水位。defaultTask 仍为最小 512 字节栈。

检查默认 ELF 没有实际 DJI/DM/VESC 电机实例或旧底盘/机构/PCcom 业务符号；
App_Init 只做 DWT/Topics 初始化和 USB 服务任务创建。
UART 接收、PWM 启动和外设解析器任务需应用显式接入，不自动产生控制动作。

## 来源与修改边界保护

迁入前后对来源文件逐个 SHA-256 比较，并比较 Git porcelain 状态。
排除 build、build-*、.cache、graphify-out 等缓存目录，含原工程 Git 元数据。
ROBOCON26_R2 的 313 个文件与原中期工程的 816 个文件均保持一致。
工具初始化曾生成的 .serena 元数据已清理，复核来源 Git 状态一致。

整个 Module/motor 共 5 个文件校验完全一致；未更改 PID/DWT/业务算法。
框架预先存在的 .vscode/settings.json 改动保留原内容，不纳入本次提交。
本次仅有 .vscode/c_cpp_properties.json 的 C++23 配置更新会提交。

| 保护项 | 基线清单 SHA-256（排序后的相对文件名与文件摘要） |
|---|---|
| ROBOCON26_R2 | 335225143f0126cf062ffb62393df899f9958900817bba29a637400a4fced099 |
| ROBOCON2027_midterm | f1d68a8a962807ff58734a9dd2de9265ae6c6ba39a60f7813dd9bc203e454f2f |
| Module/motor | 52e016441acfd1d91daa69a54418281c64dfd0b1aef36d549dc72c8959479136 |


## 契约复核与回退条件

| 契约 | 实现与验证入口 |
|---|---|
| 一次 TX 请求完整入队 | UsbPort::writeAsync；usb_backend 队列满及 512 字节复制测试 |
| 仅服务任务提交，临界区保护检查到提交 | UsbPort::ServiceOnce、CDC_Transmit_HS；真实适配器与 LL mock 检查临界区深度 |
| 端点终止前保留缓冲 | suspend/complete/abort 计数路径；挂起恢复与复位测试 |
| 关闭失败不得宣告 aborted | USBD_LL_CloseEP 检查失败进入 Error_Handler；寄存器时序未上板 |
| 协议保持格式、单任务所有 | data_packet/packet_manager/StreamTransport 与独立 CRC 参考测试 |
| 红外等待不阻塞接收处理 | emitData 写入前等待 ACK，单独 RX 任务；快速/错误 ACK 与超时测试 |
| 默认无机构动作 | 显式示例、无默认 PWM/UART 设备任务、电机文件摘要保护 |

如实机发现枚举/寄存器时序问题，先用 RC_FRAMEWORK_ENABLE_USB=OFF 回退原最小运行框架，
保留日志和复现场景；不要通过放宽类状态检查或强清 TxState 掩盖问题。
任何修改默认 UART/CAN 绑定、电机模块或帧格式的需求另立变更，不混入本次迁入。

## 实物检查清单：均未验证

- USB 枚举、虚拟串口打开/关闭、不同电脑驱动识别。
- 双向连续收发：零散块、跨帧、满 512 字节、长时间压力和队列满后的恢复。
- USB 挂起/恢复、总线复位、拔插重连、连接代次与旧帧不重放。
- VBUS sensing 关闭时的拔线检测延迟；必要时结合硬件线路另行处理。
- USB IRQ 5 与其他中断的优先级、ISR 延迟、服务任务与调用任务栈高水位。
- UART DMA 环形缓冲、Logger 连续打印、舵机 PWM 波形和实际角度。
- Xbox、红外 ACK 时序和 HWT101 与真实发送端数据对照。

未连接硬件是上述检查未执行的原因；模拟不能替代这些项目。
