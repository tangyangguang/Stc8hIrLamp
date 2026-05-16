# 验证记录

## 1. 构建

默认正式构建：

```sh
make clean && make
```

生成：

- `build/ir_lamp.ihx`
- `build/ir_lamp.map`
- `build/ir_lamp.mem`

关闭 UART 调试：

```sh
make clean && make IR_UART_DEBUG=0
```

关闭低功耗便于红外链路调试：

```sh
make clean && make POWER_DOWN_ENABLE=0
```

调整睡眠保护时间：

```sh
make clean && make POWER_DOWN_GUARD_MS=1000
```

打开详细诊断：

```sh
make clean && make IR_VERBOSE_DEBUG=1
```

关闭未用 IO 低功耗收敛：

```sh
make clean && make LOW_POWER_UNUSED_IO=0
```

## 2. 烧录和串口

在 `firmware/lamp` 目录下执行。

烧录：

```sh
<stcgal-python> -m stcgal -P stc8g -p <serial-port> -b 38400 -t 6000 build/ir_lamp.ihx
```

其中 `<stcgal-python>` 是安装了 `stcgal` 的 Python 解释器，`<serial-port>` 是 USB 串口设备，例如 macOS 下的 `/dev/cu.usbserial-110`。看到 `Waiting for MCU` 后，给目标板断电再上电。

打开串口：

```sh
pio device monitor --port <serial-port> --baud 115200
```

预期启动输出：

```text
lamp boot
```

`IR_VERBOSE_DEBUG=1` 时，关灯进入低功耗后，按遥控应先看到：

```text
lamp wake
```

普通帧预期输出：

```text
ir frame addr=0x01 cmd=0x11 action=power
ir frame addr=0x01 cmd=0x22 action=brighter
ir frame addr=0x01 cmd=0x33 action=dimmer
ir frame addr=0x01 cmd=0x51 action=timer15
ir frame addr=0x01 cmd=0x52 action=timer60
ir frame addr=0x00 cmd=0x18 action=brighter
ir frame addr=0x00 cmd=0x52 action=dimmer
```

repeat 预期输出：

```text
ir repeat action=brighter
ir repeat action=dimmer
ir repeat action=none
```

`action=none` 表示上一条普通命令不是增亮/减亮，repeat 被正确忽略。

## 3. 静态核对

- INT0 向量入口在 `src/main.c`，由 `STC8H_INTERRUPT(..., STC8H_VECTOR_INT0)` 生成。
- `build/main.lst` 显示 `0x0003` 处跳转到 `_app_int0_isr`。
- `build/main.lst` 显示 power-down 入口中 `setb 0xaf` 与 `orl 0x87, #0x02` 相邻。
- INT0 配置为双边沿。
- `build/stc8h_exti.lst` 显示配置 INT0 时清 `EX0/IE0` 后写 `IT0`，使能时置 `EX0`。
- P3.2 配置为输入，开启数字输入和上拉。
- `build/board_init.lst` 显示写 `P3IE(0xFE33) |= 0x04` 和 `P3PU(0xFE13) |= 0x04`。
- `build/board_init.lst` 显示低功耗路径会改写 `P1PU/P1IE/P3PU/P3IE`，未用脚关闭上拉和数字输入。
- `build/main.lst` 显示进入 power-down 前调用 `board_timer0_stop()`。
- 默认 `APP_SLEEP_GUARD_MS=2000`，连续快速按键时先保持清醒，停手后再进入 power-down。
- Timer0 为 12T 16-bit free-run，6 MHz 下 tick 为 2 us。
- 正式固件定义 `STC8H_TIMER0_ROLE_FREE_RUN`。
- UART1 为 115200。
- PWM 为 P1.0 / PWMA channel 1，频率 1 kHz。
- 红外解码使用 `drv_ir_rx_feed_pulse()`，按 mark/space 脉宽解析普通帧和 repeat。
- 普通帧校验地址反码和命令反码。
- repeat 只作用于增亮/减亮。
- 电源键 repeat 不触发开关；每个有效普通帧按标准命令处理一次。
- power-down 前 P1.0 拉低，INT0 保持可唤醒。
- `build/ir_lamp.map` 不应出现 `__divulong`、`__mullong`、`_stc8h_power_down`。
- `make debug-ir` 应生成 `build/debug_ir.ihx`；该目标单独启用 falling interval 解码并链接 `stc8h_exti_disable.c`。

## 4. 资源占用

默认 `IR_UART_DEBUG=1 IR_VERBOSE_DEBUG=0 POWER_DOWN_ENABLE=1`:

- ROM: 5837 bytes。
- External RAM: 27 bytes。
- Stack starts at `0x5f`，可用 161 bytes。

关闭 UART 调试 `IR_UART_DEBUG=0 POWER_DOWN_ENABLE=1`:

- ROM: 5170 bytes。
- External RAM: 27 bytes。
- Stack starts at `0x59`，可用 167 bytes。

关闭未用 IO 收敛 `LOW_POWER_UNUSED_IO=0`:

- ROM: 5815 bytes。
- External RAM: 27 bytes。
- Stack starts at `0x5f`，可用 161 bytes。

详细诊断 `IR_UART_DEBUG=1 IR_VERBOSE_DEBUG=1 POWER_DOWN_ENABLE=1`:

- ROM: 6024 bytes。
- External RAM: 38 bytes。
- Stack starts at `0x65`，可用 155 bytes。

## 5. 硬件验证记录

2026-05-11 实测 `IR_VERBOSE_DEBUG=1 POWER_DOWN_ENABLE=1`：

- 上电 UART1 115200 可输出 `lamp boot`。
- 关灯后会输出 `lamp sleep` 并进入低功耗路径。
- 按遥控后可输出 `lamp wake`，说明 P3.2 / INT0 可唤醒。
- `addr=0x01 cmd=0x11` 可解码为 `power`。
- `addr=0x01 cmd=0x22` 可解码为 `brighter`。
- `addr=0x01 cmd=0x33` 可解码为 `dimmer`。
- 长按 `0x33` 可产生 `ir repeat action=dimmer`。
- 长按 `0x22` 可产生 `ir repeat action=brighter`。
- 双边沿 mark/space 解码修复了掉电唤醒后单按只得到残缺 `ir none` 的问题。

## 6. 硬件验证清单

- 上电默认关灯。
- 上电串口输出 `lamp boot`。
- `addr=0x01 cmd=0x11` 可开关。
- `addr=0x01 cmd=0x22` 可增亮。
- `addr=0x01 cmd=0x33` 可减亮。
- `addr=0x01 cmd=0x51` 设置 15 分钟定时。
- `addr=0x01 cmd=0x52` 设置 60 分钟定时。
- `addr=0x00 cmd=0x18` 可增亮。
- `addr=0x00 cmd=0x52` 可减亮。
- 长按增亮/减亮时 repeat 连续生效且有节流。
- 长按开关和定时键不会重复触发。
- 关灯后 P1.0 为低电平。
- 关灯后整机电流进入低功耗预期范围。
- power-down 后按遥控能唤醒。
- 关灯睡眠临界点附近按键，不应出现可感知的按键吞失。
- P1.0 PWM 实测约 1 kHz。

## 7. 资料来源

- STC8H1K08 官方页面: <https://www.stcmicro.com/stc/stc8h1k08.html>
- STC8H1K08 Features PDF: <https://www.stcmicro.com/datasheet/STC8H1K08_Features.pdf>
- NEC 协议说明: <https://www.sbprojects.net/knowledge/ir/nec.php>
- NEC 协议 PDF: <https://os.mbed.com/media/uploads/zael/adoh-necinfraredtransmissionprotocol-.pdf>
- CN5711 数据手册: <https://datasheet4u.com/download_new.php?id=1257640>
