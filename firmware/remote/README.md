# STC8H 红外夜灯遥控器

本项目使用同级目录旁的 `Stc8hBase` 基础库，从零开发 STC8H1K08 红外夜灯遥控器。老 Keil 工程已归档到产品仓库 `legacy/remote_keil/`，不作为新实现的设计约束。

## 当前设计

- MCU：STC8H1K08。
- 主频：6MHz 内部 IRC。
- 红外：P1.0，PWMA CH1，实测约 37.9kHz；`APP_IR_PWM_DUTY=105` 是本板占空比校准值。
- LED：P1.1，默认启用，只在首次按键确认时短闪约 5ms。
- 按键：P3.2/P3.3/P3.6/P3.7/P3.0，外部上拉，下降沿中断唤醒。

亮度加 `0x22` 和减 `0x33` 支持标准 NEC repeat；开关 `0x11`、FN1 `0x51`、FN2 `0x52` 单发。单发键发送后有 80ms 释放确认窗口，用于改善快速连点手感。

多键同时按下时按 `POWER > UP > DOWN > FN1 > FN2` 的顺序取第一个；本遥控器按单键使用设计。

## macOS 构建

本项目按 macOS + PlatformIO + SDCC 开发，不依赖 Keil：

```sh
pio run
```

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
