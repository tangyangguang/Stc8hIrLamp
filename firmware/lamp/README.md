# STC8H 红外遥控夜灯

本项目使用同级目录旁的 `Stc8hBase` 基础库重写老项目的红外遥控夜灯功能。

硬件：

- MCU: STC8H1K08。
- LED 驱动: CN5711。
- PWM 输出: P1.0。
- 指示灯: P1.1。
- 红外接收: P3.2 / INT0。

## IO 接线

| 功能 | STC8H 引脚 | 方向 | 外部连接 | 电平/说明 |
| --- | --- | --- | --- | --- |
| 灯 PWM/CE | P1.0 / PWMA CH1 | 输出 | CN5711 CE/PWM 输入 | 1kHz PWM；关灯和掉电前拉低 |
| 状态指示灯 | P1.1 | 输出 | 指示 LED | 高电平点亮；红外命令确认时短闪 |
| 红外接收 | P3.2 / INT0 | 输入 | 红外接收头 OUT | 空闲高电平，收到 38kHz 载波时为低；双边沿捕获，支持掉电唤醒 |
| UART1 RX | P3.0 | 输入 | 调试串口 TX | 115200 8-N-1；默认调试构建启用 |
| UART1 TX | P3.1 | 输出 | 调试串口 RX | 115200 8-N-1；默认调试构建启用 |

设计文档：

- [需求设计](docs/01_REQUIREMENTS.md)
- [方案设计](docs/02_DESIGN.md)
- [验证记录](docs/03_VERIFICATION.md)

## 构建与烧录

构建：

```sh
make clean && make
```

烧录：

```sh
<stcgal-python> -m stcgal -P stc8g -p <serial-port> -b 38400 -t 6000 build/ir_lamp.ihx
```

默认开启 UART 调试和低功耗。调试红外链路时可临时关闭低功耗：

```sh
make clean && make POWER_DOWN_ENABLE=0
```

默认基础库路径为 `../../../Stc8hBase`，也可手动覆盖：

```sh
make BASE=../../../Stc8hBase
```
