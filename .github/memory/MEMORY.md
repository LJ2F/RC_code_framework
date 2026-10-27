# 框架维护约定

- 框架面向 STM32H723 + FreeRTOS，当前语言 C11/C++23。
- USB CDC 默认开启，可用 RC_FRAMEWORK_ENABLE_USB 关闭；默认不产生电机或舵机动作。
- 电机模块此次保持原内容；新增协议不包含比赛业务消息和端口绑定。
- USB 单一服务任务拥有发送缓冲；只有完成或确认端点终止才允许复用。
- PMR 保留默认动态内存路径，帧长上限不等于无堆实现。
- 修改后执行 tools/validate.py；实物限制和再生成步骤见 docs/VALIDATION.md、
  docs/COMMUNICATION.md、docs/CUBEMX_REGENERATION.md。
