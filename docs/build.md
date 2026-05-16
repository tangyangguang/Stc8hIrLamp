# 构建与烧录

## 前置目录

默认目录结构：

```text
<workspace>/
  Stc8hBase/
  Stc8hIrLamp/
```

## 直接烧录包

根目录 `flash_firmware/` 存放人工整理的最新可烧录固件和 Windows STC-ISP 烧录说明，适合没有开发环境的电脑直接烧录。

该目录是发布固件副本，可以提交到仓库；原始构建产物目录仍然不提交：

- `firmware/lamp/build/`
- `firmware/remote/.pio/`

当前发布文件：

- `flash_firmware/ir_lamp.ihx`
- `flash_firmware/remote_J3Y_LED.hex`
- `flash_firmware/remote_2TY_NoLED.hex`

## 夜灯固件

```sh
cd firmware/lamp
make clean && make
```

生成文件位于 `firmware/lamp/build/`，该目录不提交。默认烧录文件：

- `firmware/lamp/build/ir_lamp.ihx`

烧录：

```sh
cd firmware/lamp
<stcgal-python> -m stcgal -P stc8g -p <serial-port> -b 38400 -t 6000 build/ir_lamp.ihx
```

其中 `<stcgal-python>` 是安装了 `stcgal` 的 Python 解释器，`<serial-port>` 是 USB 串口设备，例如 macOS 下的 `/dev/cu.usbserial-110`。

夜灯默认构建会带 UART 调试日志，便于验收红外解码和低功耗唤醒。准备发布给最终烧录包时，优先使用关闭调试的构建以节省 ROM 并释放 UART 引脚：

```sh
cd firmware/lamp
make clean && make IR_UART_DEBUG=0
```

发布包如果继续使用默认调试构建，必须在 `flash_firmware/MANIFEST.md` 记录对应构建参数和 SHA1。

## 遥控器固件

默认 J3Y 带 LED 版本：

```sh
cd firmware/remote
pio run -t clean -e STC8H1K08
pio run -e STC8H1K08
```

2TY 无 LED 版本：

```sh
cd firmware/remote
pio run -t clean -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED
```

生成文件位于 `firmware/remote/.pio/`，该目录不提交。默认烧录文件：

- `firmware/remote/.pio/build/STC8H1K08/firmware.hex`
- `firmware/remote/.pio/build/STC8H1K08_2TY_NoLED/firmware.hex`

烧录：

```sh
cd firmware/remote
pio run -e STC8H1K08 -t upload --upload-port <serial-port>
```

2TY 无 LED 版本烧录：

```sh
cd firmware/remote
pio run -e STC8H1K08_2TY_NoLED -t upload --upload-port <serial-port>
```

## 命令差异说明

两个固件的烧录命令当前不统一，这是有意保留的：

- 夜灯使用 `SDCC + Makefile`，构建系统只负责生成 `build/ir_lamp.ihx`，烧录直接调用 `stcgal`。
- 遥控器使用 `PlatformIO + SDCC`，`upload_stcgal.py` 已接入 PlatformIO 上传流程，烧录通过 `pio run -t upload`。

不为了统一命令迁移构建系统，避免增加无关改动和验证成本。日常使用时按各自目录文档执行即可。

STC 下载需要目标板重新上电进入 bootloader。看到 `Waiting for MCU` 后，给目标板断电再上电。
