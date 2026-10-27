# RC_code_framework

基于 **STM32H723ZGT6 / Cortex-M7、STM32 HAL、FreeRTOS CMSIS-RTOS2、C11 与 C++23** 的机器人开发框架。从原竞赛工程提取，保留底层通信、设备协议和算法；默认固件初始化板载外设、DWT、消息队列，启用 USB CDC 枚举与服务任务，并运行最小 defaultTask，不创建电机实例，不启动 UART DMA 接收，不发送 CAN 控制帧。

## 目录

| 目录 | 职责 |
|---|---|
| APP/app_init | C/C++ 共用的最小应用入口 App_Init |
| BSP | CanBus / CanDevice、UartPort、DWT、DMA 缓冲属性 |
| Algorithm | 队列、双缓冲、数学工具 |
| Module/controller | PID、积分限幅、输出滤波、测量微分、增量输出 |
| Module/motor | MotorBase、DJI C610/C620、VESC、达妙 DM4310 |
| Module/imu | 维特与 HWT101 欧拉角解析，单位度 |
| Module/transfer_protocol | 校验算法、PMR 有界帧协议与通用传输适配 |
| Module/servo / logger / xbox / infrared_com | PWM 舵机、串口日志、Xbox、红外 |
| USB_DEVICE / Middlewares/ST | USB CDC 接入与 ST USB Device 中间件 |
| Module/remote | 16 字节遥控协议、CRC、按键、掉线检测 |
| Module/topics | 静态内存消息发布订阅 |
| Core / Drivers / Middlewares | CubeMX 生成代码、HAL/CMSIS、FreeRTOS |

## 工具链与构建

需要 CMake >= 3.22、Ninja、支持 C++23 的 Arm GNU Toolchain；本次验证基线为 GCC 15.2.1，将工具加入 PATH。当前 Cube 工具版本提示保存在 .settings；命令行构建不依赖 Cube 扩展。链接脚本中的 READONLY 段属性要求 GCC 11 或更新版本。

```powershell
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
```

产物在 build/Debug 或 build/Release 下：RC_code_framework.elf、.hex、.bin、.map。使用自己的 ST-Link/J-Link/DAPLink 工具烧录；仓库不包含本机绝对路径或自动烧录脚本。

## 板级配置与启动

CubeMX 文件为 RC_code_framework.ioc，使用原 STM32H723ZGT6 引脚、时钟、3 路 FDCAN、USART2/3 与 DMA 配置。两路 UART 当前为 115200 / 8N1，RX DMA 为循环模式；CAN 标称速率为 1 Mbps，经典 CAN，数据最大 8 字节。实际接口、电平、晶振和收发器连接必须与所用板卡一致；这不是跨 STM32 型号的自动适配层。

启动顺序：MPU → HAL → 时钟 → GPIO/DMA/UART/FDCAN 外设配置 → USB 单例构造与一次硬件初始化 → osKernelInitialize → MX_FREERTOS_Init（用户代码区调用 App_Init）→ osKernelStart。App_Init 只调用一次；它初始化 DWT、已注册订阅者的消息队列，并创建 USB 服务任务。USB 可通过 RC_FRAMEWORK_ENABLE_USB=OFF 关闭。defaultTask 每次延时 1 tick，当前 tick 为 1 ms。

保留启动文件中的 __libc_init_array 和链接脚本中的 .init_array，供全局 C++ 对象构造使用。DMA_BUFFER_ATTR 与 .dma_buffer 链接段配套，缓冲放在 RAM_D2，不能随意移入 DTCM。当前配置未启用 D-cache；若后续启用，需自行补充 MPU/cache 一致性处理，32 字节对齐本身不等于缓存一致。

## 接入方式

- CAN：调用者选择外设句柄、创建总线和设备，先初始化总线映射并注册设备，再配置过滤器和启动通知。BSP 已拥有 HAL CAN 回调，不要重复定义。型号协议方法保留，但具体设备 ID、减速比、PID、发送周期由应用提供。
- UART：调用者选择端口与 DMA 缓冲，内核初始化后创建信号量，显式启动接收。任务读取 UartPort::Packet 后交给协议解析器。BSP 已拥有 HAL UART 回调。接收任务使用有限等待，避免无数据时停止执行掉线检查。
- 遥控：帧为 AA 55 + 四路 little-endian uint16 + 按键 uint16 + page + color + CRC8 + DE，共 16 字节。CRC 覆盖偏移 2..13，多项式 0xD5、初始值 0。ch 保留原始 0..4095 值，不再做原工程的反向和第四通道校准；应用自行处理方向、死区、校准和动作映射。get_channel 返回相对 2048 的偏移，并非 -1..1。接收任务需周期调用 poll；跨任务缓存状态应检查 is_fresh，不能仅检查 online。
- IMU：WitMotionimu::processByte 输入字节，返回 0x53 时 getImuData 有新欧拉角。ImuData 在 IMU 模块定义；保留 pub_imu_data 别名以兼容已有调用，不依赖业务 topic。
- Topics：发布者与订阅者通常在启动前静态注册，App_Init 初始化其队列。默认最大消息 32 字节、8 个 topic、每 topic 4 个订阅者、历史深度 8。运行时新增订阅未提供独立初始化流程。覆盖环的生产端与消费端会共同更新 head，跨任务使用应在两端加适当同步；不要把当前实现当成任意多生产者并发总线。
- 电机：DM4310 当前仅实现位置+速度命令发送；枚举其他模式不代表全部已实现。VESC 当前解析指定状态帧，驱动仍有型号相关极对数/减速比假设。C620 转矩常数以原 3508 配置为基础。驱动方法不等于完整的运动安全管理，应用需提供在线检查、停机和目标限幅。DM buildTx 在显式调用时可能生成清错/使能帧；默认固件不调用它。

## 提取范围与验证

移除了四舵轮解算、底盘控制、摇杆/按键动作映射、机构电机实例、实际 PID 调参、调试任务和板载设备绑定。驱动保留原接口，仅对通用性、声明一致性和构建问题做必要调整。

尚未对提取后的固件进行实物烧录、总线收发、闭环电机或长时间调度验证。

## 通信与外设

USB 默认开启；其余外设由应用显式接入，不自动绑定 UART2/3 或创建机构任务。
UsbPort::writeAsync 仅供任务调用，单次最多 512 字节，复制整次请求后返回；
HAL_OK 表示已入队，HAL_BUSY 表示队列满，HAL_ERROR 表示未就绪或参数无效。
一个接收任务调用 Read，协议解析放在任务中。USB 服务任务统一提交发送，
挂起或断开后旧连接请求不重放。connected 表示已配置且未挂起，不表示电脑应用已打开。
VBUS sensing 关闭，不能保证立即识别每次拔线；枚举、挂起恢复、拔插与压力收发尚未实物验证。

通用协议保持 AA55、总长 BE16、命令 BE16、校验 BE16、载荷、55AA 格式。
默认总长最多 512 字节、载荷最多 502 字节；一次 UART/USB 分块不等于完整帧。
packet_manager 由单个通信任务使用，发送失败由 bool 返回值传回调用者。
PMR 仍动态分配，帧长上限不等于无堆；C++ newlib 堆与 FreeRTOS 堆分开，
无异常构建下内存耗尽仍终止。

Logger 仅供任务调用，要求 UART 有同步复制缓冲；最长输出 255 字节，
忙碌和容量不足会返回错误。打印任务或调用 writeAsync 的任务建议至少从 2 KiB 栈开始，
再上板测高水位；不要把这些调用直接加入最小 defaultTask 的 512 字节栈。
舵机的定时器、通道、角度范围、脉宽与计数频率由应用配置，必须显式调用 init 才启动 PWM。
Xbox 未配置校验回调时属于兼容模式，crcVerified 为 false；
红外等待 ACK 时需另一个接收任务持续处理输入。HWT101 的有效帧与新角度帧分别计数。

关闭 USB 时使用独立构建目录：

~~~powershell
cmake -S . -B build/usb-off -G Ninja '-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake' '-DCMAKE_BUILD_TYPE=Debug' '-DRC_FRAMEWORK_ENABLE_USB=OFF'
cmake --build build/usb-off
~~~

CubeMX 再生成后，保持 C11/C++23、USB IRQ 优先级 5，
main 中先构造 USB 单例再初始化 USB 硬件，且只初始化一次；defaultTask 不得再次初始化 USB。
保留 CDC 类状态检查、回调桥接、端点终止后的缓冲释放与发送返回值传播。

## 来源与许可

框架从 https://github.com/lxismism/ROBOCON2027_midterm 的当前工作区提取，包含提取时尚未提交的修改。自有源码保留原作者 lxlx 等署名与版权声明；本次整理未新增或替换原项目许可。原项目没有顶层开源许可证，因此公开仓库不应被理解为授予了额外的再分发许可。

第三方组件保持原目录、文件头和许可证：

| 组件 | 许可证文件 |
|---|---|
| STM32H7 HAL | `Drivers/STM32H7xx_HAL_Driver/LICENSE.txt` |
| CMSIS | `Drivers/CMSIS/LICENSE.txt` |
| STM32H7 CMSIS Device | `Drivers/CMSIS/Device/ST/STM32H7xx/LICENSE.txt` |
| FreeRTOS | `Middlewares/Third_Party/FreeRTOS/Source/LICENSE` |

各组件的实际使用条件以各自许可证和源码声明为准。CubeMX 生成代码的原始 ST 版权头保持不变。

### R2 迁入

来源为用户提供的 ROBOCON26_R2 工作区。迁入前按 SHA-256 建立完整来源基线；
没有复制比赛业务、PCcom 业务分发、Topics 业务消息或电机驱动。

| 内容 | 来源/原作者 |
|---|---|
| UsbPort、PWM 舵机 | Keten，2863861004@qq.com，2026 |
| XboxRemote | 老冯 / Keten，保留字段与主要访问接口 |
| Logger、infrared_com | zhy (Howie41)，2026 |
| Hwt101Parser | YE，2026 |
| gdut 函数包装器、数据帧、校验算法 | R2 Algorithm/function.hpp 与 Module/transfer_protocol，原文件没有明确作者署名 |
| USB Device、USB 生成代码与 HAL PCD/LL | STMicroelectronics，保持原版权头；USB 许可证见 Middlewares/ST/STM32_USB_Device_Library/LICENSE.txt |

ST usbd_cdc.c 的本地补丁传播 LL 发送返回值，
配套 usbd_conf.c 在底层失败时先终止端点；其余中间件保留来源代码。
新增功能没有替原作品增加许可证；使用者须遵守各来源已有许可和署名。
