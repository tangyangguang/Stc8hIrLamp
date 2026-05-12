# 固件烧录包

本目录存放已经从当前源码重新编译并整理好的可烧录固件，方便在没有开发环境的电脑上直接使用 STC-ISP 烧录。

`flash_firmware/` 是本仓库中允许提交的发布固件副本。构建目录仍然不提交：

- `firmware/lamp/build/`
- `firmware/remote/.pio/`

## 固件对应硬件

| 文件 | 对应硬件 | 说明 |
| --- | --- | --- |
| `ir_lamp.ihx` | 夜灯 | STC8H1K08，CN5711 灯板，红外接收 |
| `remote_J3Y_LED.hex` | J3Y 带 LED 遥控器 | P1.0 高电平发射，P1.1 LED 短闪 |
| `remote_2TY_NoLED.hex` | 2TY 无 LED 遥控器 | P1.0 低电平驱动 2TY，P1.1 不接 LED |

## Windows STC-ISP 烧录

推荐在没有开发环境的 Windows 电脑上使用 STC-ISP 图形界面：

1. 打开 STC-ISP。
2. 芯片型号选择 `STC8H1K08`。
3. 选择对应的固件文件。
4. IRC/trim 目标设置为 `6MHz`。
5. 下载波特率建议选择 `38400`。
6. 选择 USB 转串口对应的 COM 口。
7. 点击下载/编程后，给目标板断电再上电，让芯片进入 STC bootloader。

如果第一次连接失败，保持 STC-ISP 等待下载状态，再重新给目标板断电上电一次。

## 复测清单

夜灯：

- 开关
- 亮度加减
- 定时功能
- 红外唤醒和低功耗唤醒

J3Y 带 LED 遥控器：

- 5 个按键都能控制夜灯
- UP/DOWN 长按 repeat 正确
- LED 按键确认短闪
- 待机电流复测

2TY 无 LED 遥控器：

- 5 个按键都能控制夜灯
- UP/DOWN 长按 repeat 正确
- 红外发射头正极约 `37.9kHz / 1/3`
- 待机电流约 `0.6uA`
