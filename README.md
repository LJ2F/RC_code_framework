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
| examples / checks / tests | 默认不运行的接入示例、头文件编译检查、主机协议测试 |

## 工具链与构建

需要 CMake >= 3.22、Ninja、支持 C++23 的 Arm GNU Toolchain；本次验证基线为 GCC 15.2.1，将工具加入 PATH。当前 Cube 工具版本提示保存在 .settings；命令行构建不依赖 Cube 扩展。链接脚本中的 READONLY 段属性要求 GCC 11 或更新版本。

```powershell
cmake --preset Debug
cmake --build --preset Debug
cmake --preset Release
cmake --build --preset Release
cmake --build build/Debug --target framework_header_check framework_example_check
```

产物在 build/Debug 或 build/Release 下：RC_code_framework.elf、.hex、.bin、.map。使用自己的 ST-Link/J-Link/DAPLink 工具烧录；仓库不包含本机绝对路径或自动烧录脚本。

主机测试使用独立构建目录，不使用 ARM 工具链：

```powershell
cmake -S tests -B build/host -G Ninja '-DCMAKE_C_COMPILER=gcc' '-DCMAKE_CXX_COMPILER=g++'
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

## 板级配置与启动

CubeMX 文件为 RC_code_framework.ioc，使用原 STM32H723ZGT6 引脚、时钟、3 路 FDCAN、USART2/3 与 DMA 配置。两路 UART 当前为 115200 / 8N1，RX DMA 为循环模式；CAN 标称速率为 1 Mbps，经典 CAN，数据最大 8 字节。实际接口、电平、晶振和收发器连接必须与所用板卡一致；这不是跨 STM32 型号的自动适配层。

启动顺序：MPU → HAL → 时钟 → GPIO/DMA/UART/FDCAN 外设配置 → USB 单例构造与一次硬件初始化 → osKernelInitialize → MX_FREERTOS_Init（用户代码区调用 App_Init）→ osKernelStart。App_Init 只调用一次；它初始化 DWT、已注册订阅者的消息队列，并创建 USB 服务任务。USB 可通过 RC_FRAMEWORK_ENABLE_USB=OFF 关闭。defaultTask 每次延时 1 tick，当前 tick 为 1 ms。

保留启动文件中的 __libc_init_array 和链接脚本中的 .init_array，供全局 C++ 对象构造使用。DMA_BUFFER_ATTR 与 .dma_buffer 链接段配套，缓冲放在 RAM_D2，不能随意移入 DTCM。当前配置未启用 D-cache；若后续启用，需自行补充 MPU/cache 一致性处理，32 字节对齐本身不等于缓存一致。

## 接入方式

- CAN：调用者选择外设句柄、创建总线和设备，先初始化总线映射并注册设备，再配置过滤器和启动通知。BSP 已拥有 HAL CAN 回调，不要重复定义。型号协议方法保留，但具体设备 ID、减速比、PID、发送周期由应用提供。
- UART：调用者选择端口与 DMA 缓冲，内核初始化后创建信号量，显式启动接收。任务读取 UartPort::Packet 后交给协议解析器。BSP 已拥有 HAL UART 回调。示例使用有限等待，避免无数据时停止执行掉线检查。
- 遥控：帧为 AA 55 + 四路 little-endian uint16 + 按键 uint16 + page + color + CRC8 + DE，共 16 字节。CRC 覆盖偏移 2..13，多项式 0xD5、初始值 0。ch 保留原始 0..4095 值，不再做原工程的反向和第四通道校准；应用自行处理方向、死区、校准和动作映射。get_channel 返回相对 2048 的偏移，并非 -1..1。接收任务需周期调用 poll；跨任务缓存状态应检查 is_fresh，不能仅检查 online。
- IMU：WitMotionimu::processByte 输入字节，返回 0x53 时 getImuData 有新欧拉角。ImuData 在 IMU 模块定义；保留 pub_imu_data 别名以兼容已有调用，不依赖业务 topic。
- Topics：发布者与订阅者通常在启动前静态注册，App_Init 初始化其队列。默认最大消息 32 字节、8 个 topic、每 topic 4 个订阅者、历史深度 8。运行时新增订阅未提供独立初始化流程。覆盖环的生产端与消费端会共同更新 head，跨任务使用应在两端加适当同步；不要把当前实现当成任意多生产者并发总线。示例展示任务临界区。
- 电机：DM4310 当前仅实现位置+速度命令发送；枚举其他模式不代表全部已实现。VESC 当前解析指定状态帧，驱动仍有型号相关极对数/减速比假设。C620 转矩常数以原 3508 配置为基础。驱动方法不等于完整的运动安全管理，应用需提供在线检查、停机和目标限幅。DM buildTx 在显式调用时可能生成清错/使能帧；默认固件不调用它。

示例是独立 .cpp 文件，默认不链接、不创建任务。framework_example_check 只验证它们能编译。单电机示例要求该 DJI 命令分组只用于此电机，其余槽位发送零；多电机必须统一打包，避免互相覆盖。集成时显式选择需要的示例并创建应用任务；不要把整个 examples 目录自动加入固件。

## 提取范围与验证

移除了四舵轮解算、底盘控制、摇杆/按键动作映射、机构电机实例、实际 PID 调参、调试任务和板载设备绑定。驱动保留原接口，仅对通用性、声明一致性和构建问题做必要调整。

构建与协议测试结果见 [验证记录](docs/VALIDATION.md)。尚未对提取后的固件进行实物烧录、总线收发、闭环电机或长时间调度验证。

来源、原作者和第三方许可证见 [来源与许可](docs/THIRD_PARTY.md)。本仓库未替原项目新增开源许可证。

## 新增通信与外设

USB 默认开启；其余外设只提供显式接入示例。模块不绑定 UART2/3，不创建机构任务、不产生电机控制动作。
USB 单次异步发送最多 512 字节，HAL_OK 表示完整请求入队；PMR 协议载荷最多 502 字节，
发送失败可传回应用。Xbox 默认兼容模式不会声称通过 CRC 验证。
详细接口、并发约定、内存与超时语义见 [通信与外设说明](docs/COMMUNICATION.md)。

~~~powershell
# USB 关闭的独立构建目录
cmake -S . -B build/usb-off -G Ninja '-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake' '-DCMAKE_BUILD_TYPE=Debug' '-DRC_FRAMEWORK_ENABLE_USB=OFF'
cmake --build build/usb-off
# 完整构建矩阵、公共头文件、示例和主机测试
py tools/validate.py
~~~

验证脚本需要 Python 3、ARM GCC 与原生 GCC/G++ 在 PATH；日志和 JSON 结果写入 build/validation。
CubeMX 再生成检查见 [再生成说明](docs/CUBEMX_REGENERATION.md)。
