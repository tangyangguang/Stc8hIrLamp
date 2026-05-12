# STC8H 红外夜灯遥控器

本项目使用同级目录旁的 `Stc8hBase` 基础库，从零开发 STC8H1K08 红外夜灯遥控器。老 Keil 工程已归档到产品仓库 `legacy/remote_J3Y_LED_keil/` 和 `legacy/remote_2TY_NoLED_keil/`，不作为新实现的设计约束。

## 当前设计

- MCU：STC8H1K08。
- 主频：6MHz 内部 IRC。
- 红外：P1.0，PWMA CH1，实测约 37.9kHz；`APP_IR_PWM_DUTY=105` 是本板占空比校准值。
- 默认硬件：J3Y/S8050 NPN 低边驱动红外发射头，高电平发射，P1.1 LED 启用。
- 兼容硬件：2TY 无 LED 早期遥控器，P1.0 低电平驱动 2TY，红外发射头正极高电平发射，P1.1 LED 反馈关闭。
- 按键：P3.2/P3.3/P3.6/P3.7/P3.0，外部上拉，下降沿中断唤醒。

## IO 接线

### J3Y 带 LED 版本

| 功能 | STC8H 引脚 | 方向 | 外部连接 | 电平/说明 |
| --- | --- | --- | --- | --- |
| 红外发射 | P1.0 / PWMA CH1 | 输出 | J3Y/S8050 NPN 基极电阻，低边驱动红外发射管 | P1.0 输出约 37.9kHz PWM；高电平导通发射，空闲拉低 |
| 指示 LED | P1.1 | 输出 | 指示 LED | 高电平点亮；首次按键确认时短闪约 5ms |
| POWER | P3.2 / INT0 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x11`，单发 |
| UP | P3.3 / INT1 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x22`，支持 NEC repeat |
| DOWN | P3.6 / INT2 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x33`，支持 NEC repeat |
| FN1 | P3.7 / INT3 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x51`，单发 |
| FN2 | P3.0 / INT4 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x52`，单发 |

### 2TY 无 LED 版本

| 功能 | STC8H 引脚 | 方向 | 外部连接 | 电平/说明 |
| --- | --- | --- | --- | --- |
| 红外发射 | P1.0 / PWMA CH1 | 输出 | 2TY 三极管驱动红外发射管 | P1.0 低电平驱动 2TY，空闲拉高；红外发射头正极约 37.9kHz，正极高电平占空比约 1/3 |
| 指示 LED | P1.1 | 输出 | 未接 LED | `STC8H1K08_2TY_NoLED` 环境关闭 LED 反馈 |
| POWER | P3.2 / INT0 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x11`，单发 |
| UP | P3.3 / INT1 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x22`，支持 NEC repeat |
| DOWN | P3.6 / INT2 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x33`，支持 NEC repeat |
| FN1 | P3.7 / INT3 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x51`，单发 |
| FN2 | P3.0 / INT4 | 输入 | 按键到 GND，外部上拉 | 按下为低电平；命令 `0x52`，单发 |

亮度加 `0x22` 和减 `0x33` 支持标准 NEC repeat；开关 `0x11`、FN1 `0x51`、FN2 `0x52` 单发。单发键发送后有 80ms 释放确认窗口，用于改善快速连点手感。

多键同时按下时按 `POWER > UP > DOWN > FN1 > FN2` 的顺序取第一个；本遥控器按单键使用设计。

## macOS 构建

本项目按 macOS + PlatformIO + SDCC 开发，不依赖 Keil：

J3Y 带 LED 版本：

```sh
pio run -t clean -e STC8H1K08
pio run -e STC8H1K08
pio run -e STC8H1K08 -t upload --upload-port <serial-port>
```

2TY 无 LED 版本：

```sh
pio run -t clean -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED -t upload --upload-port <serial-port>
```

其中 `<serial-port>` 是 USB 串口设备，例如 macOS 下的 `/dev/cu.usbserial-110`。看到 `Waiting for MCU` 后，给目标板断电再上电。

## 基础库引用

`src/base/*.c` 通过 `#include "../../../../../Stc8hBase/..."` 引用基础库实现，没有复制基础库代码。`platformio.ini` 配置以下 include path：

- `include`
- `board/stc8h1k08_ir_lamp_remote`
- `../../../Stc8hBase/core`
- `../../../Stc8hBase/hal`
- `../../../Stc8hBase/drivers`
- `../../../Stc8hBase/utils`

烧录配置中的系统时钟应与 `include/app_config.h` 中的 `STC8H_SYSCLK_HZ` 保持一致。

板级配置已启用基础库编译期裁剪：GPIO 仅 P1/P3，PWM 仅 PWMA CH1。
