# RC_code_framework

基于 **STM32H723ZGT6 / Cortex-M7、STM32 HAL、FreeRTOS CMSIS-RTOS2、C11 与 C++23** 的机器人开发框架。从原竞赛工程提取，保留底层通信、设备协议和算法；默认固件初始化板载外设、DWT、消息队列，启用 USB CDC 枚举与服务任务，并运行最小 defaultTask，不创建电机实例，不启动 UART DMA 接收，不发送 CAN 控制帧。

## 目录

| 目录 | 职责 |
|---|---|
| APP/app_init | C/C++ 共用的最小应用入口 App_Init |
| BSP | CanBus / CanDevice、UartPort、DWT、DMA 缓冲属性、轻量 H7 I2C 适配 |
| Algorithm | 队列、双缓冲、数学工具、可选固定尺寸矩阵/向量 |
| Module/kalman | 标准卡尔曼滤波 |
| Module/controller | R2 原版 PID，替换原框架 PID |
| Module/movebase | R2 原版 S 曲线规划器、独立底盘运动学 |
| Module/PCcom | 保留原结构的 UART/USB 上位机通信，业务入口留空 |
| Module/motor | MotorBase、DJI C610/C620、VESC、达妙 DM4310 |
| Module/imu | 维特与 HWT101 欧拉角解析，单位度 |
| Module/transfer_protocol | 校验算法、PMR 有界帧协议与通用传输适配 |
| Module/servo / logger / xbox / infrared_com | PWM 舵机、PCA9685、串口日志、Xbox、红外 |
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


## 可选 TLSF 内存池与 FreeRTOS 同步

面向当前 STM32H723 + C++23 工程，提供固定容量池、诊断统计、显式 PMR 适配和可选同步。
原始池、PMR 和 get_state 共用池内同一把可选锁；不更换全局 new/delete、
FreeRTOS 堆或默认 PMR 资源，不自动扩容、创建池/任务，不改变现有 DMA 缓冲或通信资源路径。

| 配置 | 行为 |
|---|---|
| TLSF=OFF、TLSF_FREERTOS=OFF（默认） | 不编译 TLSF 组件 |
| TLSF=ON、TLSF_FREERTOS=OFF | 核心、池、PMR、通用锁接口；默认构造不加锁，也可注入自定义锁 |
| TLSF=ON、TLSF_FREERTOS=ON | 额外编译独立的 FreeRTOS 静态互斥锁后端 |
| TLSF=OFF、TLSF_FREERTOS=ON | 配置报错，避免无效组合 |

两个完整开关名是 RC_FRAMEWORK_ENABLE_TLSF、RC_FRAMEWORK_ENABLE_TLSF_FREERTOS，默认均 OFF。
它们与 USB、矩阵开关独立，状态保存在各构建目录的 CMake 缓存中。

```powershell
cmake --preset Debug -DRC_FRAMEWORK_ENABLE_TLSF=ON -DRC_FRAMEWORK_ENABLE_TLSF_FREERTOS=ON
cmake --build --preset Debug
cmake --preset Release -DRC_FRAMEWORK_ENABLE_TLSF=ON -DRC_FRAMEWORK_ENABLE_TLSF_FREERTOS=ON
cmake --build --preset Release
```

只使用单任务池时关闭 TLSF_FREERTOS；完全关闭时两个选项都设为 OFF。
启用 FreeRTOS 后端不要求联网下载。CubeMX 再生成后保留根 CMake 的开关、宏定义和
include(cmake/tlsf.cmake)，沿用当前 FreeRTOS include/port 配置。

### 原始池与 PMR

`Algorithm/memory/tlsf_pool.hpp/.cpp` 保留使用者的 Tlsf_Pool 类名和小写接口：

- `Tlsf_Pool pool;` 不加锁，要求单任务独占；`Tlsf_Pool pool(&lock);` 借用可选锁，
  不允许运行时换锁，锁须覆盖池的生命周期。
- `init(buffer, bytes)` 借用调用者真实可写的固定区域；分别检查控制结构创建和加入区域。
  拒绝重复初始化；容量包含管理开销，不等于用户可申请总量。
- `try_allocate(bytes, alignment)` 失败返回 nullptr；零长度、非法对齐、未初始化、
  超大请求和耗尽均可报告失败。释放须回原池且仅一次，deallocate(nullptr) 无操作。
- 分配得到原始存储，不自动构造对象；deallocate 不析构对象，也不验证错误指针。
  支持显式 add_region 加入区域；不支持移除、重置、池复制/移动、自动扩容或上游堆兜底。
- init/add_region/try_allocate/deallocate/is_initialized/get_state 的状态读取、计数更新及 TLSF 操作
  均处于同一个 guard 内；内部直接检查 handle_，避免嵌套获取非递归锁。
- 析构要求独占、所有调用和分配已结束；不能用析构加锁来允许并发销毁。
  锁仅保护分配器，不自动同步容器或用户数据。

`Tlsf_Memory_Resource` 借用池引用，显式传给 std::pmr 容器：

```cpp
#include "tlsf_memory_resource.hpp"
#include <vector>

void single_task_example() {
    alignas(std::max_align_t) static std::byte arena[16 * 1024];
    Tlsf_Pool pool;
    if (!pool.init(arena, sizeof(arena))) {
        return;
    }
    Tlsf_Memory_Resource resource(pool);
    {
        std::pmr::vector<int> values(&resource);
        values.reserve(32);
        values.push_back(42);
    } // 缓冲归还池，之后才允许销毁 resource 和 pool
}
```

例子的 16 KiB 只是使用示意，不代表默认固件预留了这块 RAM。
容器复制也应显式传资源；clear 不表示释放容量。元素内部的其他普通分配不自动改用本池。
零字节资源申请按一字节处理，资源按对象身份比较相等，资源禁止复制/移动；
本例一个池使用一个资源对象。存储、池和资源必须覆盖全部使用者生命周期。

PMR 分配失败不是正常的 nullptr 返回：无异常工程中先执行可选 noexcept 故障回调，
若回调返回则 std::abort()，不把空指针交给容器。需要恢复的操作继续使用原始接口。
池返回失败前 guard 已解锁，故障回调不在池锁内执行；回调避免再次分配内存。

### FreeRTOS 静态互斥锁

`tlsf_freertos_lock.hpp/.cpp` 提供 Tlsf_FreeRTOS_Lock。
对象内置 StaticSemaphore_t，经 xSemaphoreCreateMutexStatic 显式创建标准 mutex，
不从 FreeRTOS 堆分配控制块，不使用二值信号量替代 mutex；使用内核的优先级继承机制。

- 构造不调用 RTOS。lock.init() 在发布给其他任务前独占调用，可在调度器启动前创建；
  重复初始化、ISR/挂起态、运行任务中屏蔽中断或创建失败返回 false。
- **带此后端的池操作必须在调度器运行的普通任务中进行。**
  即使 mutex 已在启动前创建，也不能把 pool.init() 放到全局构造或当前 App_Init
  （它在 osKernelStart 前运行）中；应在初始化任务中完成，再发布就绪通知。
- lock 使用 portMAX_DELAY 等待；要求 INCLUDE_vTaskSuspend=1，不忙等，也不把竞争报成内存不足。
- 未初始化、ISR、调度器未运行/挂起、PRIMASK/BASEPRI/FAULTMASK 屏蔽状态、
  同任务递归获取、非持有者释放或底层 take/give 错误均 std::abort()。
- init 与析构必须独占，锁长于池；析构时不能有持有者或等待者，静态句柄由 vSemaphoreDelete 清理。
  不得在持锁期间强制删除任务；RAII 不承诺 abort、HardFault 或直接删任务时执行析构。

下面只是应用接入示意，不编入默认应用：

```cpp
#include "tlsf_freertos_lock.hpp"
#include "tlsf_memory_resource.hpp"
#include "main.h"
#include <vector>

namespace {
alignas(std::max_align_t) std::byte memory[16 * 1024];
Tlsf_FreeRTOS_Lock memory_lock;
Tlsf_Pool memory_pool(&memory_lock);
Tlsf_Memory_Resource memory_resource(memory_pool);
}

// 由一个普通运行任务调用一次，然后再通知其他任务“内存池已就绪”。
void memory_init_in_task() {
    if (!memory_lock.init() ||
        !memory_pool.init(memory, sizeof(memory))) {
        Error_Handler();
    }
}

// 就绪之后各任务使用自己的容器，共享此 resource；同一容器需另行同步。
void use_memory_from_task() {
    std::pmr::vector<int> values(&memory_resource);
    values.reserve(32);
    values.push_back(42);
}
```

后端独立目标 rc_framework_tlsf_freertos 才依赖当前板级/FreeRTOS 头文件；
核心与通用锁接口仍可主机编译。需要 static allocation、mutex、
scheduler-state/holder 查询和无限等待支持，当前 FreeRTOSConfig.h 已具备。
本轮不修改 UART/CAN、业务任务、FreeRTOS 堆、Topics 或默认通信资源。

### 统计与开销

get_state() 返回 tlsf_pool_state：initialized、capacity_bytes、free_bytes、
used_block_bytes、largest_free_block、free_block、used_block、
allocation_failures、peak_used_block_bytes、region_count。

- 容量是初始上界；已用量按 TLSF 内部块大小统计，不等于用户请求长度，
  块头开销使 free_bytes + used_block_bytes 不必等于 capacity_bytes。
- 失败次数按对象生命周期累计，覆盖未初始化、非法参数、超大请求和底层失败，
  每次调用仅计一次，在 size_t 最大值饱和；成功初始化不清零。
- 峰值按已用块内部大小之和累计，释放后不降低，不包含全部管理开销。
- get_state 是 O(块数)，持锁覆盖完整遍历，低频调用；不能把容器扩容、复制、
  锁等待和遍历统计整体当作固定执行时间。
- CPU 存储与 DMA 能力分开；本轮没有引入动态 DMA 池或缓存一致性方案。

### 最大区域容量与控制结构裁剪

RC_FRAMEWORK_TLSF_MAX_POOL_SIZE 默认 65536（64 KiB），通过 PUBLIC 宏
TLSF_MAX_POOL_SIZE 同时传给 C 核心、池封装和使用者。它限制的是每次 init/add_region
传入区域的总字节数，不是所有区域的容量总和；首段还包含唯一的 TLSF 控制结构。

支持十进制 1024..1073741824；0 恢复上游架构相关索引范围。
非 2 的幂容量采用向上取整的索引位数，但 init/add_region 仍按精确字节上限检查。
这个值是整个 TLSF 库的编译配置，不是每个对象独立的参数，改变后须重新构建。
超过限制会在写入候选区域前返回 false；过小首段仍可能因控制结构开销而被拒绝。

```powershell
# 最大单段为 4 KiB；应用示例的区域也要相应缩小。
cmake --preset Debug -DRC_FRAMEWORK_ENABLE_TLSF=ON -DRC_FRAMEWORK_TLSF_MAX_POOL_SIZE=4096
cmake --build --preset Debug
```

上面的 16 KiB 用法示例适用于默认 64 KiB 配置；区域大小必须与当前上限匹配。
关闭 TLSF 不预留这些空间；64 KiB 只是允许的单段上限，不是默认占用量。

ARM32 控制结构由 Cortex-M7 编译后的符号大小核对（不是实机耗时测量）：

| 单段上限配置 | 一级索引数量 | TLSF 控制结构 |
|---|---:|---:|
| 0，保留上游范围 | 24 | 3188 B |
| 65536，默认 | 10 | 1340 B |
| 16384 | 8 | 1076 B |
| 6000，非 2 的幂 | 7 | 944 B |
| 4096 | 6 | 812 B |
| 1024 | 4 | 548 B |

上述数值只统计 C 核心控制结构。当前 Tlsf_Pool 对象另占 ARM32 128 B，
包含 8 段静态登记表；锁、PMR 资源、每段边界及每个分配块还各有开销。
公共控制结构只在首段占用一次；池对象内登记表不占用借入区域，也不从上游堆分配。

### 同一控制结构加入多个区域

init 建立首段及公共控制结构；add_region 显式加入后续区域。
首段只支付一次控制结构开销，后续段各自仍有池边界和块管理开销。

```cpp
#include "tlsf_pool.hpp"

namespace {
alignas(std::max_align_t) std::byte region_a[4096];
alignas(std::max_align_t) std::byte region_b[4096];
Tlsf_Pool pool; // 单任务；多任务时注入同一把已初始化的锁。
}

bool initialize_regions() {
    if (!pool.init(region_a, sizeof(region_a))) {
        return false;
    }
    return pool.add_region(region_b, sizeof(region_b));
}
```

以上数组只是接口示例，没有指定物理内存区。应用可以通过链接脚本/section 放到不同
CPU 可访问区域；本次不自动预留 DTCM、AXI SRAM 或 DMA 专用池。

- 最多 8 段（含首段），由 Tlsf_Pool::max_regions 表示；登记表在池对象内，不动态分配。
- 加入前检查初始化状态、空地址、对齐、容量、地址加法溢出、区域重叠和登记上限。
  首段的控制结构地址范围也受保护；失败不修改候选区域或现有分配。
- 已有分配允许继续存活，显式加入后仍有效；不提供移除、重置或自动加段。
- 所有段共用同一 TLSF 控制结构、资源和可选锁；add_region 也持这把锁。
  FreeRTOS 后端要求在普通运行任务中调用，且必须先完成独占初始化和就绪通知。
- get_state 的 region_count 返回已登记段数；容量、空闲量和块数汇总所有段，
  largest_free_block 取所有段中最大的单个空闲块。
- 单次分配不能拼接两个区域；即使地址相邻也保持独立边界。
  总空闲量大于请求长度，不代表存在足够大的连续块。
- 不按区域的速度或 DMA 属性选择内存；不能把“可用于 DMA 的段”和“仅 CPU 可用的段”
  混在同一个资源中，再假设分配结果必定适合 DMA。32 字节对齐也不保证缓存一致性。
- 每一段、池、锁和资源须覆盖全部分配生命周期；不得交给其他分配器重复管理。
  默认程序仍不实例化这些示例对象。

### 来源、验证与限制

原始 TLSF 3.1 来自 [mattconte/tlsf](https://github.com/mattconte/tlsf)，固定提交
deff9ab509341f264addbd3c8ada533678591905，与原 robocon-library 引用一致。
核心以该提交为基线，现已加入容量裁剪与边界检查补丁；版权/BSD 三条款原文见 tlsf.h；版本/blob 校验和换行保护见
Middlewares/Third_Party/TLSF/SOURCE.md。使用者手写池、统计、PMR 和 guard 的署名保留。
第四轮由助手直接补齐统一同步与 FreeRTOS 后端；没有给原作品新增或替换许可。

本机验证材料只放临时目录，不恢复 checks/tests/examples/tools/docs。
构建、主机回归与 FreeRTOS API 边界模拟的实际结果在本节末尾记录。
模拟使用主机互斥，不执行真实 ARM 调度器，不能代替实物调度/优先级继承验证。

容量裁剪与多区域接入已实现；后续尚未实施通信模块显式注入资源。实机内存与时序测量仍待完成。
未上板验证：多任务长期压力、锁等待/优先级继承时序、任务栈高水位及错误停机路径。



### 第四轮同步验收（2026-10-30）

- Arm GNU GCC 15.2.1、C11/C++23，全新 Debug/Release × USB 开/关 ×
  TLSF 关闭/核心开启/FreeRTOS 后端开启，共 12 组构建通过；矩阵关闭。
- 拒绝 TLSF=OFF 而 TLSF_FREERTOS=ON 的非法配置。
- 主机 GCC 15.2.0、无异常/RTTI，Debug/Release 基础池、计数、快照、guard、
  原始接口/PMR 共享锁以及 PMR 故障回调回归通过。
- 两类四线程压力检查分别覆盖通用主机锁和实际后端经模拟 FreeRTOS API 运行；
  统计/计数一致，全部释放后合并恢复，故障回调执行前池锁已释放。
- 后端静态创建/删除、启动前创建、重复初始化、创建失败重试及无效 init 上下文通过；
  每种构建覆盖 13 条终止路径：未初始化 lock/unlock、未启动/挂起、ISR、
  三类中断屏蔽、take/give 错误、递归、非持有者释放及持有时析构。
- Debug/Release 各链接一个真实引用 FreeRTOS 后端、池和 PMR vector 的 ARM 测试固件，
  使用现有内核和板级对象；测试固件仅在临时目录，未替换默认产物或烧录。
- 同一构建类型和 USB 状态下，三种开关配置未实例化时的最终 text/data/bss 相同。
- 来源工程、电机、其他业务与编辑器修改保持原样，未暂存、提交或推送。
  实物调度、优先级继承/锁等待时序和栈峰值未测。

### 容量裁剪与多区域验收（2026-10-30）

- Arm GNU GCC 15.2.1：Debug/Release × USB 开/关 × TLSF 关闭/核心/FreeRTOS 后端
  的 12 组默认配置，加 Debug/Release 4 KiB、Debug 非整次幂 6000 B、
  Release 原范围 0，共 16 组全新固件构建通过，编译警告为 0；矩阵关闭。
- 5 个额外 ARM 固件实际保留 add_region、池、PMR 和真实 FreeRTOS 后端符号并链接；
  仅放临时目录，未替换默认产物或烧录。未实例化时，各模式默认固件 text/data/bss 相同。
- PUBLIC 最大容量定义已检查传播到 C 核心、C++ 池、FreeRTOS 后端及固件使用者。
- 主机 GCC 15.2.0：Debug/Release × 65536/4096/6000/1024/0，共 10 组多区域检查通过：
  满容量边界、精确非整次幂限制、错误地址/溢出/对齐、重叠与控制结构保护、
  加入失败不写内存、登记上限、已有分配保持、逐段耗尽/复用、相邻独立边界、
  所有段一致性/汇总统计、随机读写，以及四线程原始分配/PMR/统计与显式加段。
- 原有基础池、统计、计数、guard、PMR 故障策略和 FreeRTOS API 模拟回归均通过；
  Debug/Release 各覆盖 13 条同步终止路径；额外验证真实后端源码经模拟 API 管理两段内存。
- ARM 编译验证 8 个容量配置，并拒绝 3 个非法核心定义；CMake 拒绝 4 个非法容量输入。
- 主机核心继承上游 default_walker 的格式诊断，严格检查时仅对该第三方核心禁用 format 诊断；
  自写 C++ 使用 Wall/Wextra/Werror/pedantic。没有为本轮改写上游诊断输出。
- 检查程序和构建材料仅放临时目录或忽略的 build，不恢复 checks/tests/examples/tools/docs。
- 尚未验证 STM32 实物上的不同 RAM 区域、长期调度、优先级继承时序或 DMA/Cache 行为。
  分配器不推断物理内存属性。当前不提交、推送，也不自动实例化池或改通信资源路径。

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
| TLSF 3.1（可选） | `Middlewares/Third_Party/TLSF/tlsf.h` 内 BSD 三条款许可原文 |

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

## 标准卡尔曼、独立底盘运动学与 PCA9685

这三组模块按最小改动迁入，不自动创建任务、设备对象或输出控制信号。
卡尔曼与底盘运动学依赖现有矩阵后端，使用时按“可选矩阵计算”一节开启
`RC_FRAMEWORK_ENABLE_MATRIX`；该选项默认值保持不变，本轮没有增加模块开关。

### 标准卡尔曼

`Module/kalman/kalman_filter.hpp` 提供 `gdut::kalman_filter<N, L, M, T>`：
N 是状态维数，L 是控制输入维数，M 是观测维数，T 默认为 float。
保留原构造、预测/校正、矩阵配置与状态读取接口；只提取标准版本，去掉未使用的旧内存资源、
deque 和 PMR 头文件，不迁入自适应版本。`Algorithm/uncopyable.hpp` 为原样复制的依赖。

~~~cpp
#include "kalman_filter.hpp"

void filter_step() {
    using Mat = gdut::matrix<float, 1, 1>;
    gdut::kalman_filter<1, 1, 1> filter(
        Mat{1.0f}, Mat{0.0f}, Mat{1.0f}, Mat{0.1f}, Mat{0.2f});
    filter.update_prediction(Mat{0.0f});
    [[maybe_unused]] auto estimate = filter.update_correction(Mat{1.0f});
}
~~~

上例仅演示一轮计算；连续估计时由应用保存同一个滤波器对象。
默认观测矩阵为零，实际使用必须配置符合模型的 A/B/H/Q/R。
算法仍调用原 inverse()，调用者须保证创新协方差可逆；沿用现有矩阵模块在 Debug
断言、Release 返回零矩阵的求逆失败行为，本次没有改写数值算法或恢复策略。
矩阵存储固定且不申请堆内存；对象和中间矩阵随维数增长，放入任务前需要核算栈空间。

### 独立底盘运动学

`Module/movebase/chassis_kinematics.hpp` 只计算车体速度与四轮速度之间的变换：

| 原接口 | 几何参数 |
|---|---|
| `gdut::universal_wheel_kinematics<Radius>` | 轮子到车体中心的距离，单位米 |
| `gdut::chassis_kinematics<Radius>` | 上一类的原别名 |
| `gdut::mcanum_wheel_kinematics<Width, Length>` | 保留原拼写，使用 `mid = (Width + Length) / 2` |

~~~cpp
#include "chassis_kinematics.hpp"

void kinematics_step() {
    using Chassis = gdut::chassis_kinematics<0.3f>;
    gdut::vector<float, 3> velocity{1.0f, 0.0f, 0.0f};
    auto wheels = Chassis::forward_kinematics(velocity);
    [[maybe_unused]] auto restored = Chassis::inverse_kinematics(wheels);
}
~~~

车体输入顺序为 vx、vy、omega，原注释单位为 m/s、m/s、rad/s，输出沿用原轮速单位 m/s。
Radius 不是轮子自身半径，结果也不是电机 RPM。俯视车头朝上，+vx 向前、+vy 向左、
+omega 逆时针；全向轮正方向沿逆时针切向，麦克纳姆轮正方向统一对应向前。
两套模型现在统一采用以下输入/输出轮序：

| 向量分量 | 数组下标 | 轮子位置 |
|---|---|---|
| v1 | wheels[0] | 右前 |
| v2 | wheels[1] | 左前 |
| v3 | wheels[2] | 左后 |
| v4 | wheels[3] | 右后 |

全向轮正解按原 v4/v3/v2/v1 重排行，逆解同步重排列；原数值系数和麦克纳姆轮矩阵保持不变。
电机正负号、安装方向、轮径/RPM 换算由应用按实物核对，轮序不是 CAN 电机 ID。
本模块没有绑定电机、PID、S 曲线、Topics 或遥控策略。

### PCA9685 与轻量 H7 I2C 适配

`Module/servo/bsp_pca9685.hpp/.cpp` 保留 `gdut::pca9685(gdut::i2c&, address)`
以及原寄存器、PWM、舵机、休眠/唤醒接口。驱动仅将 F4 HAL 头文件替换为 H7，
移除未使用的 clock.hpp；寄存器操作、参数裁剪和计算逻辑保持原样。

`BSP/bsp_iic.hpp` 从原 I2C 封装提取构造、mem_write、mem_read、is_device_ready、
超时转换和不可复制/移动约定，切换到本框架的 H7 HAL。它只借用 HAL 句柄，
不包含原 F4 外设配置、DMA 或中断回调体系，不初始化总线或接管引脚。

~~~cpp
#include "bsp_pca9685.hpp"

// 调用前，由应用完成 H723 I2C 时钟、引脚、Timing 和 HAL 句柄初始化。
HAL_StatusTypeDef configure_servo(I2C_HandleTypeDef& handle) {
    gdut::i2c bus(&handle);
    gdut::pca9685 pwm(bus);
    auto status = pwm.init(50.0f);
    if (status != HAL_OK) {
        return status;
    }
    return pwm.set_servo_angle(0, 90.0f);
}
~~~

上例只有被显式调用时才配置芯片并设置输出。构造函数本身不访问硬件；
init() 成功后按原实现将全部通道关闭，再由应用设置需要的通道。
设备默认 7 位地址为 0x40，传入驱动的是已左移的 HAL 地址 0x80。
16 通道共用频率；保留原内部 25 MHz 假定、频率裁剪、12 位占空值及舵机脉宽换算。
相关硬件定义见 [NXP PCA9685 数据手册](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)。

这是原阻塞 I2C 路径，读写超时仍为 20 ms，初始化含 HAL_Delay；
在任务上下文使用。同一总线多使用者由应用串行化，句柄、总线适配对象须覆盖使用者生命周期。
空句柄返回 HAL_ERROR，HAL_BUSY/HAL_TIMEOUT/错误状态直接返回。
输入约束沿用原实现，调用者应提供有限数值；本轮没有增加非有限数值处理或新的控制功能。
当前框架没有预设 I2C 实例，CubeMX 引脚、时钟、Timing、上拉及 PCA9685 的 OE/供电需按实物接入。

### 来源与本轮验证

来源：[gdut-robocon11-second-group/robocon-library](https://github.com/gdut-robocon11-second-group/robocon-library/tree/d21b3ccf822901dd22ba1e319298a8bdae3eaff7)，
固定提交 d21b3ccf822901dd22ba1e319298a8bdae3eaff7。
卡尔曼与 uncopyable 来自 Components；底盘与 PCA9685 来自 Modules；
轻量 I2C 来自 `BSP/bsp_iic.hpp` 的必要子集。保留原注释与声明，没有新增或替换原作许可。

- Arm GNU GCC 15.2.1、C11/C++23：Debug/Release 各验证“矩阵关、USB 开”和“矩阵开、USB 关”，4 组全新配置构建通过、零编译警告。
- 另有 Debug/Release 两组矩阵与 USB 同时开启的 ARM 固件，强制保留并链接新模块实际引用；通过且零编译警告。测试入口未由启动代码调用。
- 5 个新增公共头文件分别以真实 H723 编译选项进行独立及重复包含检查，通过。
- 主机 GCC 15.2.0、C++23、无异常/RTTI：Debug/Release 的标量与 6 状态卡尔曼 float/double 参考计算、全向轮及麦克纳姆轮正逆往返检查通过。
- PCA9685 HAL 模拟覆盖初始化顺序、寄存器/地址/超时传递、PWM/舵机端点与限幅、空句柄及 27 条 HAL 错误/忙碌/超时与后续恢复路径。模拟检查不代表真实总线时序已验证。
- 验证程序、日志仅存本机临时目录，构建产物在忽略的 build 目录；没有恢复 checks/tests/examples/tools/docs。
- 未连接硬件：I2C 应答、实际 PWM 频率/脉宽、OE 行为、舵机动作及真实底盘方向与尺度均未验证。PID、电机、S 曲线和 TLSF 保持本轮前内容。
