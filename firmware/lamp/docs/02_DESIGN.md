# 红外遥控夜灯方案

## 1. 分层

本项目只承担板级和产品逻辑：

- `board/`: P1.0、P1.1、P3.2、Timer0、UART1、power-down 前后 IO 状态。
- `src/app_ir.*`: NEC 事件转产品命令，处理 repeat 策略。
- `src/app_light.*`: 开关、亮度、定时、指示灯反馈和 PWM 输出。
- `src/main.c`: 初始化、INT0 向量、主循环调度、低功耗入口。

`Stc8hBase` 负责芯片级能力：

- GPIO 模式。
- INT0 配置。
- Timer0 12T free-run。
- UART1。
- PWM。
- NEC mark/space 脉宽解码。

本项目启用基础库通用裁剪宏以适配 STC8H1K08 8KB flash：

- `STC8H_GPIO_PORT_MASK=0x0A`: 只保留 P1/P3 分支。
- `STC8H_PWM_CHANNEL_MASK=0x01`: 只保留 PWMA channel 1。
- `STC8H_TIMER_ENABLE_1MS=0`: 不编译 Timer0 1ms 初始化。
- `STC8H_TIMER_ENABLE_TIMER0_FREE_RUN=1`: 保留 Timer0 12T free-run。
- `STC8H_TIMER_ENABLE_TIMER0_RESET=1`: 保留唤醒后 Timer0 reset。
- `STC8H_TIMER_ENABLE_RUN_CONTROL=1`: 保留 Timer start/stop。
- `STC8H_TIMER_ENABLE_INTERRUPT_CONTROL=0`: 不编译 Timer 中断控制 API。
- `STC8H_UART_ASSUME_UART1=1`: UART HAL 只按 UART1 使用。
- `STC8H_UART_ENABLE_WRITE_RAM=0`: 只保留 code string 输出。
- `STC8H_UART_ENABLE_RX=0`: 不编译 UART 轮询接收。
- `DRV_IR_RX_ENABLE_PULSE=1`: 保留 NEC mark/space pulse 解码。
- `DRV_IR_RX_ENABLE_FALLING=0`: 不编译 falling interval 解码。

本次评估未发现阻塞本项目的基础库 bug。GPIO 上拉和数字输入使能目前在板级代码中通过 STC8H SFR 设置，属于板级电气配置；后续若多个项目复用，可再沉淀为基础库 API。

## 2. 红外接收

红外接收头输出空闲为高，收到载波时为低。INT0 配置为双边沿触发。

中断路径只做短操作：

1. 读取 Timer0。
2. 读取 P3.2 当前电平。
3. 将原始边沿输入交给 `app_ir_feed_edge_isr()`。

`app_ir_feed_edge_isr()` 继续完成：

1. 将输入电平映射为 mark/space。
2. 计算与上次边沿的 tick 差。
3. 转成 us。
4. 将上一段 mark/space 脉宽喂给 `drv_ir_rx_feed_pulse()`。

主循环负责：

1. 检测红外空闲超时。
2. 读取 `drv_ir_rx_get_event()`。
3. 映射产品命令。
4. 对空闲超时后的残缺帧复位解码器。

这样避免 old_prj 的 10 us 固定 Timer0 中断，只在红外边沿到来时进入中断。双边沿捕获也能避免 power-down 唤醒时丢失第一条下降沿后无法解出当前帧的问题。

## 3. 命令映射和 repeat

普通 NEC 帧通过地址和命令映射到产品命令。只有映射成功的普通帧会交给灯状态机。

repeat 行为：

- 普通帧为增亮/减亮时，记录为 repeat 候选。
- 普通帧为开关或定时时，清空 repeat 候选。
- repeat 帧只复用增亮/减亮候选。
- 灯状态机对 repeat 做 100 ms 节流。
- repeat 不触发指示灯闪烁。
- 电源键 repeat 不触发开关；每个有效普通帧按标准命令处理一次。

## 4. 灯状态机

状态数据：

- `light_on`: 当前开关状态。
- `level_index`: 当前亮度档位。
- `countdown_sec`: 定时关灯剩余秒。
- `second_ms`: 秒计数。
- `repeat_ms`: repeat 节流。
- `led_flash_ms`: 指示灯反馈。
- `timer_feedback_ms`: 定时确认短灭反馈。

开灯：

- 使能 PWM。
- 按当前亮度输出。
- 清除定时。

关灯：

- 禁止 PWM。
- P1.0 拉低。
- 清除定时。

定时：

- 仅开灯状态有效。
- 设置倒计时后短暂关闭 PWM，再恢复输出。

## 5. PWM 和 CN5711

P1.0 使用 PWMA channel 1，频率为 1 kHz。

1 kHz 的选择：

- 满足 CN5711 资料中 CE/PWM 低于 2 kHz 的约束。
- 在 6 MHz 下周期计数约 6000，15 档亮度分辨率充足。
- 关灯和低功耗前禁止 PWM 并拉低 P1.0，避免 CE 悬空或误亮。

## 6. UART 调试

默认开启：

```sh
make
```

关闭调试并移除 UART HAL：

```sh
make clean && make IR_UART_DEBUG=0
```

打开红外脉宽和低功耗诊断日志：

```sh
make clean && make IR_VERBOSE_DEBUG=1
```

预期日志：

```text
lamp boot
ir frame addr=0x01 cmd=0x11 action=power
ir frame addr=0x00 cmd=0x18 action=brighter
ir repeat action=brighter
ir repeat action=none
```

`IR_VERBOSE_DEBUG=1` 时会额外输出 `lamp sleep`、`lamp wake` 和 `ir none ...`。

中断内不输出串口日志。

## 7. 低功耗

默认启用：

```sh
make
```

关闭低功耗便于红外调试：

```sh
make clean && make POWER_DOWN_ENABLE=0
```

调整关灯/唤醒后的睡眠保护时间，默认 2000 ms：

```sh
make clean && make POWER_DOWN_GUARD_MS=1000
```

关闭未用 IO 低功耗收敛，用于排查硬件扩展脚冲突：

```sh
make clean && make LOW_POWER_UNUSED_IO=0
```

进入 power-down 的条件：

- 灯关闭。
- 红外解码不活跃。
- 关灯/唤醒保护时间结束，默认 2000 ms。这个延迟用于改善连续快速按键手感；停手后仍进入长期 power-down。
- P3.2 当前为空闲高电平。

进入前：

- P1.0/P1.1 保持运行态推挽输出并拉低。
- P3.2 配置为高阻输入。
- 开启 P3.2 数字输入和上拉。
- 停止 Timer0。
- P1 未用脚配置为高阻输入，关闭上拉和数字输入。
- P3 未用脚配置为高阻输入，关闭上拉和数字输入。
- UART1 的 P3.0/P3.1 在 power-down 前也按未用脚处理；唤醒后如开启调试，会重新初始化 UART1。
- 清除 INT0 标志，保持 INT0 使能。
- 全局中断关闭期间完成睡眠准备，进入 power-down 前若 INT0 标志已置位或 P3.2 已经为低，则取消本次睡眠。
- 真正进入 power-down 时，`EA=1` 与 `PCON|=PD` 在同一段入口代码中相邻执行，减少唤醒边沿落在两者之间的窗口。

唤醒后：

- 恢复运行态 IO。
- 重新启动 Timer0；红外唤醒 ISR 会先重置 Timer0，保证唤醒帧测时重新开始。
- 重新初始化 UART1。
- 保持短活动窗口，接收完整红外帧或 repeat。

## 8. 资源策略

- 不使用固定 10 us 定时中断。
- Timer0 只作为 free-run 时间基准。
- 业务在主循环处理。
- 中断只做捕获和最小状态推进。
- 不引入新依赖。
