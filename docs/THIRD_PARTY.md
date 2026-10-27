# 来源与许可

框架从 https://github.com/lxismism/ROBOCON2027_midterm 的当前工作区提取，包含提取时尚未提交的修改。自有源码保留原作者 lxlx 等署名与版权声明；本次整理未新增或替换原项目许可。原项目没有顶层开源许可证，因此公开仓库不应被理解为授予了额外的再分发许可。

第三方组件保持原目录、文件头和许可证：

| 组件 | 许可证文件 |
|---|---|
| STM32H7 HAL | `Drivers/STM32H7xx_HAL_Driver/LICENSE.txt` |
| CMSIS | `Drivers/CMSIS/LICENSE.txt` |
| STM32H7 CMSIS Device | `Drivers/CMSIS/Device/ST/STM32H7xx/LICENSE.txt` |
| FreeRTOS | `Middlewares/Third_Party/FreeRTOS/Source/LICENSE` |

各组件的实际使用条件以各自许可证和源码声明为准。CubeMX 生成代码的原始 ST 版权头保持不变。

## 本次 R2 迁入

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

本次调整记录在 docs/COMMUNICATION.md。ST usbd_cdc.c 的本地补丁传播 LL 发送返回值，
配套 usbd_conf.c 在底层失败时先终止端点；其余中间件保留来源代码。
新增功能没有替原作品增加许可证；使用者须遵守各来源已有许可和署名。
