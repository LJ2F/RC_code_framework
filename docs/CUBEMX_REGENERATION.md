# CubeMX 再生成检查

RC_code_framework.ioc 保留既有 UART2/3、FDCAN1/2/3 和 DMA 配置，新增 USB_DEVICE CDC、
OTG_HS Device Only FS、PA11/12、HSI48 和优先级 5。
固件包名称沿用来源工程记录，不声明本机安装了该包；命令行构建使用仓库中的 HAL/CMSIS 和中间件。
.settings 中保留已有工具版本设置，不虚构与 Arm GNU 15.2.1 对应的 Cube 插件包版本。

每次生成后检查：

1. 保持 C11/C++23、C++ 扩展关闭、无 RTTI/异常，恢复顶层 cmake/usb.cmake 和自有源文件列表。
2. main.c USER CODE 2 中先 UsbPort_Prepare 再 MX_USB_DEVICE_Init，仅调用一次；
   defaultTask 中若生成了第二次 USB 初始化，删除那一次。
3. 时钟启用 HSI48，USB 时钟源 48 MHz；OTG_HS 内置 PHY 设 Full Speed、
   DMA 关闭、VBUS sensing 关闭；IRQ 优先级 5。不要改变原 UART/CAN 配置。
4. App_Init 仍在 osKernelInitialize 后初始化 DWT、Topics 并检查 USB 服务任务创建结果。
   USB 关闭时不能链接 USB 中间件、PCD 或 USB 示例。
5. 恢复 usbd_cdc_if.c 的参数/设备/类状态检查和同一临界区内提交；
   使用 pClassDataCmsit[classId]。恢复 CDC Init/DeInit/RX/TX complete 桥接。
6. 恢复 usbd_conf.c 复位、挂起、恢复、断开事件和底层提交失败后的端点终止处理。
   CDC 类 DeInit 只有关闭端点后才能报告 aborted。
7. 恢复 ST usbd_cdc.c 的 LL 返回值传播补丁。不得只清 TxState 来释放正在发送的缓冲。
8. 保持链接段 .dma_buffer 在 RAM_D2、32 字节对齐；保持启动 .init_array。
9. 重新跑 Debug/Release × USB ON/OFF、头文件/示例检查和所有主机测试。
   对照 docs/VALIDATION.md 完成实物测试后再用于机器人。

USB 插拔和时钟配置涉及板级线路，此文档不表示完成实物验证。
