# RC_code_framework

基于 **STM32H723ZGT6 / Cortex-M7、STM32 HAL、FreeRTOS CMSIS-RTOS2、C11 与 C++23** 的机器人开发框架。从原竞赛工程提取，保留底层通信、设备协议和算法；默认固件初始化板载外设、DWT、消息队列，启用 USB CDC 枚举与服务任务，并运行最小 defaultTask，不创建电机实例，不启动 UART DMA 接收，不发送 CAN 控制帧。

## 目录

| 目录 | 职责 |
|---|---|
| APP/app_init | C/C++ 共用的最小应用入口 App_Init |
| BSP | CanBus / CanDevice、UartPort、DWT、DMA 缓冲属性 |
| Algorithm | 队列、双缓冲、数学工具、可选固定尺寸矩阵/向量 |
| Module/controller | R2 原版 PID，替换原框架 PID |
| Module/movebase | R2 原版 S 曲线规划器 |
| Module/PCcom | 保留原结构的 UART/USB 上位机通信，业务入口留空 |
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


## 可选矩阵计算

`Algorithm/matrix.hpp` 提供 C++23 固定尺寸的 `gdut::matrix<float/double, Rows, Cols>`
和列向量 `gdut::vector<float/double, Rows>`。支持加减、矩阵/标量乘法、转置、
行列式、求逆、单位矩阵/常量矩阵、范数/归一化、点积/叉积和 4×4 缩放、平移、旋转矩阵。
存储为行优先；`a[row, col]` 是 C++23 多参数下标，列向量也支持 `v[index]`。
只支持 float 和 double，尺寸在编译期检查；不创建任务、不分配动态内存。

默认关闭。需要使用时重新配置：

```powershell
cmake --preset Debug -DRC_FRAMEWORK_ENABLE_MATRIX=ON
cmake --build --preset Debug
cmake --preset Release -DRC_FRAMEWORK_ENABLE_MATRIX=ON
cmake --build --preset Release
```

CMake 会自动提供矩阵头文件路径，并编译本仓库附带的 CMSIS-DSP 最小矩阵后端；
无需联网下载，也无需启用其他外设。关闭时使用 `-DRC_FRAMEWORK_ENABLE_MATRIX=OFF`，
不编译或链接该后端。开关状态保存在各自构建目录的 CMake 缓存中。

```cpp
#include "matrix.hpp"

void calculate() {
    using Mat2 = gdut::matrix<float, 2, 2>;
    const Mat2 a{4.0f, 7.0f, 2.0f, 6.0f};
    const auto product = a * a.transpose();
    Mat2 inverse;
    if (a.try_inverse(inverse)) {
        const float first = inverse[0, 0];
        (void)first;
    }
    (void)product;
}
```

- 推荐 `try_inverse(out)`：奇异矩阵、非有限输入或产生非有限逆矩阵时返回 false，
  保留 out 原值；支持输入输出为同一对象，不改变独立的输入矩阵。
- `inverse()` 保留兼容接口：失败时触发 assert；关闭断言时返回零矩阵。
  `is_invertible()` 通过尝试求逆判断，避免用固定行列式阈值误判缩放后的小矩阵。
  这些接口不估计条件数，病态矩阵仍需应用按精度要求处理。
- 默认构造清零；`build_but_not_clean_mat` 不初始化元素，使用前必须完整赋值。
  范数使用 hypot 避免直接平方导致的溢出；零向量归一化仍为零，
  零旋转轴产生单位矩阵，旋转角单位为弧度。
- 数组、运算结果和求逆/LU 分解临时缓冲都可能占用任务栈。
  单个矩阵数据为 `Rows * Cols * sizeof(T)` 字节；矩阵大小和任务栈需一起规划。
  支持 double 不等于已验证实时性能。
- 此后端仅包含矩阵包装实际调用的 10 个 CMSIS-DSP C 函数和 6 个依赖头文件，
  不代表提供完整 CMSIS-DSP API。Cortex-M7/FPU/浮点 ABI 沿用工程工具链，
  没有复制来源仓库的 Cortex-M4 配置。

来源：矩阵包装迁自
[robocon-library/DSP/matrix.hpp](https://github.com/gdut-robocon11-second-group/robocon-library/blob/d21b3ccf822901dd22ba1e319298a8bdae3eaff7/DSP/matrix.hpp)，
基线提交 `d21b3ccf822901dd22ba1e319298a8bdae3eaff7`。
原文件没有单独的作者/许可证头，本次保留原声明和注释，注明来源，不为其另加许可证。
本地适配增加了直接依赖头、尺寸检查、可检查失败的求逆及数值边界处理。

CMSIS-DSP 来自 [ARM-software/CMSIS-DSP v1.16.2](https://github.com/ARM-software/CMSIS-DSP/tree/v1.16.2)，
提交 `d5717e454fec0337bef114a21f1d2d01d74f2701`。
附带文件保持上游内容和版权头；Apache-2.0 许可证见
`Middlewares/Third_Party/CMSIS-DSP/LICENSE`。

### 矩阵模块验证记录（2026-10-28）

| 项目 | 结果 |
|---|---|
| 固件全新构建 | Arm GNU GCC 15.2.1，Debug/Release × 矩阵开/关 × USB 开/关，共 8 组通过 |
| 主机数值检查 | GCC 15.2.0，Debug 13,545 项、Release 13,557 项，全部通过；检查期间没有 C++ 堆分配 |
| 数学与失败路径 | float/double、小矩阵及 CMSIS 大矩阵路径、矩形乘法/转置、LU 行列式、主元交换、求逆、缩放小矩阵、奇异/非有限输入、向量和变换 |
| 编译约束 | 非浮点元素、零维度、超出 CMSIS 尺寸范围及乘法维度不匹配均被拒绝 |
| 目标架构检查 | Cortex-M7 C++23 接口实例化、10 个后端函数可重定位链接通过，无未解析的矩阵后端符号 |
| 未使用时的固件占用 | 同一构建类型和 USB 配置下，矩阵开/关的 Flash、RAM 使用量相同 |
| 来源与修改保护 | 17 个 CMSIS-DSP 文件与上游 Git blob 校验一致；两个来源工程和框架电机模块内容、来源工程 Git 状态及暂存区保持不变 |

验证程序和脚本放在本机临时目录，没有重新加入 checks、tests、examples、tools 或 docs。
尚未上板执行矩阵运算，也未测量实际执行时间及任务栈峰值。


## R2 PID、S 曲线和 PCcom

三个模块直接加入 CMake 源码列表，不增加模块开关。应用按需包含头文件、
实例化和调用；默认应用不创建这些对象或新增任务。

- `Module/controller/pid_controller.h/.c`：用 R2 原文件替换旧 PID，
  不改算法、结构体、枚举或初始化逻辑。使用 `PID_t`、`PID_Init`、
  `PID_Calculate`、`PID_Reset`，输出上限字段为 `MaxOut`。
  PID 对象先清零，参数由应用填写；计算仍使用原 DWT 时间路径。
- `Module/movebase/pathplanning.h/.cpp`：按 R2 原文件复制，
  保留八参数 `SCurvePlanner` 构造、`update(dt)` 返回速度、
  `progress()` 和 `reset(offset)`。未重写规划公式或边界处理。
- `Module/PCcom/PCcom.hpp/.cpp`：保留原类、UART/USB 构造和
  `init/ProcessRx/ProcessTx/OnPacket`。删除比赛命令、Topics、夹爪、
  二维码和导航业务；`OnPacket` 与 `ProcessTx` 留空，由应用填写。
  没有加入新的通用回调接口。

PCcom 先构造再显式调用 `init()`；同一个通信任务调用 `ProcessRx()`。
公开的 `send(code, msg)` 模板放在头文件，仍按原方式发送对象字节；
字段布局、字节序和对端格式由应用约定。发送返回底层是否接受本次请求，
不代表对端已收到。UART 保留阻塞 `write`，USB 使用框架现有的
`writeAsync`，受已有 USB 开关控制。帧协议、PMR 内存路径保持不变。

为匹配 R2 PID，经确认只把 DJI/VESC 两个头文件内共 6 处
`.Maxout` 引用改为 `.MaxOut`，其余电机内容未改。
来源署名和原版权头保留；PID 与 S 曲线四个文件经字节比对与 R2 一致。

验证（2026-10-28）：Arm GNU GCC 15.2.1、C++23，
Debug/Release × USB 开/关四组全新构建通过；另两组 ARM 实例化链接
实际引用 PID、S 曲线和 PCcom，并编译全部电机头文件，通过。
PCcom 主机模拟检查覆盖原 UART 阻塞路径、USB 路径、发送状态回传、
最大载荷/超长拒绝和接收队列消费。测试放在本机临时目录，
没有恢复 checks、tests、examples、tools、docs。
本次没有验证原 PID/S 曲线的控制性能、轨迹精度或实物收发。

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
| CMSIS-DSP v1.16.2 矩阵子集 | `Middlewares/Third_Party/CMSIS-DSP/LICENSE` |
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
| PID | 大帅将军，R2 pid_controller.h/.c 原文件，2026 |
| SCurvePlanner | 冯大帅将军，R2 pathplanning.h/.cpp 原文件，2025 |
| PcCom | R2 Module/PCcom，原文件没有独立作者/许可证头 |
| gdut 函数包装器、数据帧、校验算法 | R2 Algorithm/function.hpp 与 Module/transfer_protocol，原文件没有明确作者署名 |
| USB Device、USB 生成代码与 HAL PCD/LL | STMicroelectronics，保持原版权头；USB 许可证见 Middlewares/ST/STM32_USB_Device_Library/LICENSE.txt |

ST usbd_cdc.c 的本地补丁传播 LL 发送返回值，
配套 usbd_conf.c 在底层失败时先终止端点；其余中间件保留来源代码。
新增功能没有替原作品增加许可证；使用者须遵守各来源已有许可和署名。
