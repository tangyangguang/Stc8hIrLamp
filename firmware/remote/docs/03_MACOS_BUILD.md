# macOS 构建

本项目按 macOS + PlatformIO + SDCC 开发，不依赖 Keil。

## 构建

默认 J3Y 带 LED 版本：

```sh
pio run -t clean -e STC8H1K08
pio run -e STC8H1K08
```

生成文件位于 `.pio/build/STC8H1K08/`。

2TY 无 LED 版本：

```sh
pio run -t clean -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED
```

生成文件位于 `.pio/build/STC8H1K08_2TY_NoLED/`。

## 烧录

默认 J3Y 带 LED 版本：

```sh
pio run -t upload --upload-port <serial-port>
```

2TY 无 LED 版本：

```sh
pio run -e STC8H1K08_2TY_NoLED -t upload --upload-port <serial-port>
```

其中 `<serial-port>` 是 USB 串口设备，例如 macOS 下的 `/dev/cu.usbserial-110`。STC 下载需要目标板重新上电进入 bootloader。看到 `Waiting for MCU` 后，给目标板断电再上电。

当前上传配置使用 PlatformIO 自带的 `tool-stcgal`，协议 `stc8g`，下载波特率 `38400`，并通过 `custom_stcgal_trim = 6000` 把 IRC 设置为 6MHz。若出现切换波特率后 `read timeout`，先确认是否按要求在 `Waiting for MCU` 后重新上电；仍失败时再临时把 `custom_stcgal_baud` 降到 `9600` 排查串口链路。

## 频率

`include/app_config.h` 中 `STC8H_SYSCLK_HZ` 必须与芯片实际 IRC 频率一致。当前配置为 `6000000UL`，烧录日志中的 `Target frequency` 或 `Trimming frequency` 应接近 6MHz。

## 构建资源

2026-08-20 在当前基础库 PWM 通用 API 下全量重新构建：

- J3Y 带 LED：Flash `4191 / 8192` bytes（51.2%），无 XDATA 分配，栈可用 197 bytes。
- 2TY 无 LED：Flash `4171 / 8192` bytes（50.9%），无 XDATA 分配，栈可用 197 bytes。

两种构建均仅保留 PWMA CH1；占空比只写入已校验范围，PWM 热路径不保留通道检查、占空比钳位或周期/预分频 RAM 镜像。

## 硬件验证记录

2026-05-12 实测：

- 使用本项目遥控器固件和本项目夜灯固件，频繁快速单击开关键，未再复现开关键偶尔不响应的问题。
- J3Y 带 LED 遥控器使用 2 节 7 号干电池供电，按键操作结束后进入待机，待机电流约 `1.05uA`。
- 2TY 无 LED 遥控器 5 个按键可用，UP/DOWN repeat 正确。
- 2TY 无 LED 遥控器待机电流约 `0.6uA`。
- 2TY 无 LED 遥控器红外发射头正极实测约 `37.9kHz`，正极高电平占空比约 `1/3`。
