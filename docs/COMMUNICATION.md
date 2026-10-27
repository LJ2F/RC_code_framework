# 通信与外设接入

## 通用帧协议

源自 R2 的 gdut::data_packet、packet_manager、固定存储 function，保留 PMR 默认内存资源。
完整帧为 AA 55 | 总长 BE16 | 命令 BE16 | 校验 BE16 | 载荷 | 55 AA。
默认完整帧 512 字节，头 8、尾 2、载荷最多 502。统一编译定义 RC_FRAMEWORK_MAX_FRAME_SIZE 可改变协议上限；
USB 的单次发送能力仍固定 512，调大协议上限不会调大 USB 队列元素。

crc16_algorithm 使用原反射多项式 0xA001（正常表达 0x8005）、初始值 0xFFFF，
跳过偏移 6、7 两个校验字节，覆盖其余整个帧，结果大端存放。checksum_algorithm 仍可独立使用。
与遥控器使用的 CRC8 协议不同。verification_algorithm.hpp 不依赖 USB、HAL 或 Topics。

接收逐字节填充，最多保留一个不超过上限的候选帧；保留跨输入块半包，
长度错误、错误 CRC、错误尾部后丢一个字节重新寻找包头。合法长度的半包会等待后续数据，
本层没有时间戳，应用需要时可调用 reset_receive() 丢弃超时半包。
body_end() 不含包尾。构造超长包得到无效对象，send() 返回 false。
发送回调签名改为 bool(const uint8_t*,const uint8_t*)；false 会传回 send()。

StreamTransport 把 uint16_t 命令码和 span 载荷交给应用回调，不认识机器人业务。
载荷视图仅在回调期间有效。一个实例由一个通信任务操作，禁止在 ISR 内做 PMR 分配或协议解析，
不保证跨任务并发安全。函数包装器默认固定存储 16 字节；使用捕获 this/单指针的小回调，
较大的捕获应通过用户上下文指针传递。

PMR vector 仍动态分配，512 字节约束是帧长上限，不是无堆实现。
默认资源走 C++ new/newlib _sbrk 堆；FreeRTOS 的 configTOTAL_HEAP_SIZE=15360 是另一块堆，
不能用 xPortGetFreeHeapSize 估算 C++ 剩余内存。无异常构建下分配失败会终止，
不改写全局分配器。频繁收发会产生临时包分配，但接收缓冲容量不会随总输入无限增长。
可在应用设置合适的 PMR 资源；本次保留默认路径。

## USB CDC

默认启用；CMake 设置 RC_FRAMEWORK_ENABLE_USB=OFF 可完整移除 USB 中间件和 USB 示例的编译。
OTG_HS 控制器使用内置 PHY，以 Full Speed 工作，PA11/PA12，HSI48 48 MHz。
接口名中的 HS 不代表当前硬件以 High Speed 运行。USB IRQ 优先级 5，允许 RTOS 通知。
保持 R2 的 VBUS sensing 关闭设置，不要求 DTR；软件不能保证立即检测每一次物理拔线。

应用仅调用 UsbPort::Instance()：
- writeAsync(data,len)：任务中调用，1..512 字节，复制整次请求；HAL_OK 表示入队，
  HAL_BUSY 表示队列满，HAL_ERROR 表示断开/挂起/非法参数/ISR 调用。电脑收到需应用确认协议。
- Read(Packet&)：一个接收任务调用，Packet 最多 256 字节，带连接 generation。
  SetRxCallback 的回调在 Read 内执行，配置回调与读取应由同一接收任务完成。
- connected()：已配置、类实例有效且未挂起；不表示电脑应用已打开串口。
- txBusy()：发送中、有等待重试的数据或队列非空。getStats() 是各原子计数的快照，
  并非跨所有字段的同一时刻事务快照。计数为 uint32_t，允许自然回绕。

RX/TX 各 8 项静态队列；一次 TX 请求对应一个队列元素，不拆成若干元素部分入队。
队列空间不包含正在发送的 staging，一个服务任务独占 CDC 提交。中断复制 RX 并记录通知，
不续发、不分配、不格式化。USBD_BUSY 保留当前缓冲，用通知和 1 tick 周期重试。
服务任务普通优先级、栈 1024 字节；创建失败进入 Error_Handler。接收解析任务由应用显式创建。
USB 单例在硬件初始化前完成构造，早期 IRQ 不会访问未创建的 RTOS 对象。

CDC_Transmit_HS 在同一短临界区检查参数、配置状态、classId、
pClassDataCmsit[classId] 和 TxState，再设置缓冲、提交；不解引用未经检查的 pClassData。
原始底层接口不供应用直接调用。中间件本地补丁会传播 LL 提交错误；
LL 失败关闭端点后再允许释放缓冲，异常提交错误需要重新枚举，必要时拔插设备。
挂起只使新请求失败并淘汰旧连接队列数据，不会单独释放硬件仍引用的缓冲。
发送完成或端点终止事件后服务任务才复用缓冲；不通过强行清 TxState 来伪造完成。
断开、复位、反初始化、挂起和恢复改变连接代次，旧 TX/RX 不自动重放。
USB 示例还在代次变化时清空协议半包，避免跨连接拼包。

## UART、Logger 与舵机

UartPort::txCopyCapacity() 只读返回 DMA 每次可同步复制的长度，不改变原有发送语义。
零表示没有复制缓冲，writeDma 借用调用者内存，此时 Logger 拒绝发送。
Logger 最长格式化 255 字节，超长截断；空格式、格式化失败或容量不足返回 HAL_ERROR，
忙碌返回 HAL_BUSY；被等级过滤返回 HAL_OK，表示按配置跳过。默认等级门限 255。
Logger 示例 TX 双缓冲总长 512 字节、单块 256。同 UART 的多个发送者必须由应用串行化，
Logger 仅在任务中使用。完整 512 字节协议帧需要 UART TX 双缓冲总长至少 1024。

PM20sServo 通过 TIM 句柄、通道和 Config 注入角度范围、脉宽范围及计数频率（经过预分频后的 Hz）。
检查参数与 ARR，有限角度自动限幅，NaN/Inf 拒绝。init() 返回 HAL PWM 启动结果；
构造和默认固件均不会启动 PWM。定时器周期、GPIO 和实际舵机参数由应用提供，
不得照搬某个机器人的占空比和机械角度。

## Xbox、红外与 IMU

Xbox 保留 FC FB 01 1C + 28 字节载荷 + 2 字节校验 + FD FE；
16 个按键字节，后续 6 个大端 uint16 摇杆/扳机值。拒绝 ID 错误和非 28 载荷，
支持重叠包头恢复。兼容模式未配置校验回调时 crcVerified() 为 false，
不能把通过包头/长度/尾部检查称为 CRC 验证。setChecksum 接收完整 36 字节及上下文，
发送端的 CRC 字节序与覆盖范围必须在自定义回调中确定。
online(now_ms) 默认 350 ms 超时，setTimeout 可调整，时间为毫秒并支持 uint32 回绕。
getControllerData 在一个任务内读取；跨任务共享由应用同步。

红外 TX 为 FA F1 地址1 地址2 数据；正常 RX 为连续的三个原始字节；ACK 为 F1。
Message 由模块拥有，不依赖 Topics，不按旧比赛的地址和值过滤。
接收任务支持半包和连包；原协议无帧头或 CRC，丢字节后无法无歧义自动重同步，
等待 ACK 期间的下一字节被按 ACK 处理，发送端需避免该时刻混入正常数据。
emitData 在写串口之前进入等待状态，处理快速 ACK，timeout 参数是毫秒，
内部转换到内核 tick。需要另一个接收任务持续读取 UART，不能在同一任务等待自己的 ACK。
init 只设置模块状态，不自动绑定端口或启动 DMA。

Hwt101Parser 放在小写 Module/imu，与现有维特驱动并存。processByte 返回校验通过的帧，
lastFrameIsAngle 区分新 0x53 角度帧；frameCount 只计角度，validFrameCount 统计所有有效帧。
yawDeg 单位度，保留原有有符号 16 位转换。所有解析均在接收任务内完成。

示例默认不链接：protocol_uart_example.cpp、protocol_usb_example.cpp、logger_example.cpp、
servo_example.cpp、device_protocol_example.cpp。每个 UART 的消费者和 HAL 回调只保留一份，
不要同时接入多个占用同一个 UART 的示例。

## 栈与关闭失败

writeAsync 会在调用任务栈上构造约 520 字节的请求快照，Logger 也有 256 字节格式化缓冲。
接入这些接口的应用任务建议从至少 2 KiB 栈开始，再用最坏调用链和实机高水位确认；
不要直接在默认 512 字节 defaultTask 栈上叠加这些示例。
USB 服务任务保持 1 KiB，本次编译栈报告可供参考，不能替代实机高水位验证。

USBD_LL_CloseEP 检查 HAL 关闭结果。ST CDC 本身忽略关闭结果，因此关闭失败必须进入
Error_Handler 停止执行，防止继续 DeInit 后错误宣告 aborted 并复用仍有引用的缓冲。
默认 PCD DMA 关闭，关闭端点也屏蔽该端点中断；本次不扩展到 USB DMA 缓存一致性处理。
connection_changes 统计状态事件（包括复位/类初始化等），不是物理插拔次数。
