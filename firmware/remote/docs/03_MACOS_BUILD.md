# macOS 构建

本项目按 macOS + PlatformIO + SDCC 开发，不依赖 Keil。

## 构建

```sh
pio run
```

生成文件位于 `.pio/build/STC8H1K08/`。

## 烧录

```sh
pio run -t upload --upload-port /dev/cu.usbserial-110
```

STC 下载需要目标板重新上电进入 bootloader。看到 `Waiting for MCU` 后，给目标板断电再上电。

当前上传配置使用 PlatformIO 自带的 `tool-stcgal`，协议 `stc8g`，下载波特率 `38400`，并通过 `custom_stcgal_trim = 6000` 把 IRC 设置为 6MHz。若出现切换波特率后 `read timeout`，先确认是否按要求在 `Waiting for MCU` 后重新上电；仍失败时再临时把 `custom_stcgal_baud` 降到 `9600` 排查串口链路。

## 频率

`include/app_config.h` 中 `STC8H_SYSCLK_HZ` 必须与芯片实际 IRC 频率一致。当前配置为 `6000000UL`，烧录日志中的 `Target frequency` 或 `Trimming frequency` 应接近 6MHz。
