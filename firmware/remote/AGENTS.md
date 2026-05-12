# 项目规则

- 本项目是 STC8H1K08 红外夜灯遥控器，使用相邻目录中的 `Stc8hBase` 基础库从零开发。
- 老 Keil 工程已归档到产品仓库 `legacy/remote_J3Y_LED_keil/` 和 `legacy/remote_2TY_NoLED_keil/`，只作为历史归档，不作为设计约束；不要为了兼容旧实现背负包袱。
- 本项目只能修改本项目文件，不能修改 `Stc8hBase`；若发现基础库 bug 或能力缺失，先给出基础库完善提示词。
- 基础库必须通过引用接入：`src/base/*.c` wrapper include `../../../../../Stc8hBase/...`，不要复制基础库源码。
- 开发环境是 macOS + PlatformIO + SDCC，不使用 Keil 工程作为交付目标。

## 当前硬件与配置

- MCU：STC8H1K08。
- 主频：6MHz 内部 IRC；`STC8H_SYSCLK_HZ`、`board_build.f_cpu`、stcgal trim 必须一致。
- 烧录默认：`stc8g`、`38400`、`custom_stcgal_trim = 6000`。
- 红外输出：P1.0，PWMA CH1，6MHz 下 `period=158`，实测约 37.9kHz；默认 `STC8H1K08` 环境为 J3Y 带 LED，高电平发射。
- `STC8H1K08_2TY_NoLED` 环境用于 2TY 无 LED 版本，低电平发射，并关闭 LED 反馈。
- 本板 PWM 占空比已实测校准：`APP_IR_PWM_DUTY=105`，不要改回理论 1/3 的 53；53 实测高电平约 64%。
- LED：P1.1，默认启用，只在首次按键确认时短闪约 5ms，不跟随 38kHz 载波。
- 按键外部上拉，按下为低电平，通过 INT0-INT4 唤醒。
- 基础库裁剪：GPIO 只保留 P1/P3，PWM 只保留 PWMA CH1；未编译 Timer/UART/IR RX 通用模块时不要额外添加无效裁剪噪声。

## 按键行为

- POWER：P3.2 / INT0 / `0x11`，单发。
- UP：P3.3 / INT1 / `0x22`，完整 NEC 帧 + 标准 repeat。
- DOWN：P3.6 / INT2 / `0x33`，完整 NEC 帧 + 标准 repeat。
- FN1：P3.7 / INT3 / `0x51`，单发。
- FN2：P3.0 / INT4 / `0x52`，单发。
- 多键同时按下按 `POWER > UP > DOWN > FN1 > FN2` 取第一个；本遥控器按单键使用设计。
- 单发键必须 one-shot：发送后最多等待 80ms 确认释放；若仍按住则不重复发送，可进入掉电。
- UP/DOWN repeat：完整帧后约 40ms 发第一次 repeat，后续帧后等待 96ms，使 repeat 起点间隔约 108ms。

## 时序与低功耗

- NEC 码元和应用阻塞等待都使用基础库 `stc8h_delay_timer0_1t_us()`，不要链接或调用粗略 `stc8h_delay_us/ms()`。
- Timer0 当前专用于红外微秒级阻塞延时；不要再拿 Timer0 做应用 tick，除非重新设计资源分配。
- 空闲前关闭 IR 载波、拉低 IR 输出、关闭 LED，再进入 power-down。
- 亮度键长按期间不进入掉电，因为需要维持 repeat 和检测松手。
- 未用 IO 暂不盲目配置；若空闲电流异常，再结合原理图处理。

## 验证要求

- 修改后至少运行 `pio run` 和 `./tools/host_syntax_check.sh`。
- 烧录后检查 stcgal 日志中频率接近 6MHz。
- 硬件验收包括：5 个键码值、UP/DOWN repeat、单发键按住只发一次、LED 首次短闪、空闲掉电、按键唤醒。
- 若红外异常，先确认载波约 38kHz、占空比约 1/3、芯片频率与编译配置一致，再判断是否是基础库问题。
