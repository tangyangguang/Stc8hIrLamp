# Stc8hIrLamp

STC8H 红外遥控夜灯产品仓库，包含夜灯固件、遥控器固件和老 Keil 工程完整归档。

## 目录

- `firmware/lamp`: 红外遥控夜灯固件，使用 SDCC + Makefile。
- `firmware/remote`: 红外夜灯遥控器固件，使用 PlatformIO + SDCC。
- `legacy/lamp_keil`: 夜灯老 Keil 工程完整归档。
- `legacy/remote_J3Y_LED_keil`: J3Y 带 LED 遥控器老 Keil 工程完整归档。
- `legacy/remote_2TY_NoLED_keil`: 2TY 无 LED 遥控器老 Keil 工程完整归档。
- `docs`: 产品级构建、依赖和归档说明。

## 基础库

本仓库不复制 `Stc8hBase`。默认要求目录布局如下：

```text
<workspace>/
  Stc8hBase/
  Stc8hIrLamp/
```

依赖版本记录见 [docs/dependency.md](docs/dependency.md)。

## 构建与烧录

夜灯：

```sh
cd firmware/lamp
make clean && make
<stcgal-python> -m stcgal -P stc8g -p <serial-port> -b 38400 -t 6000 build/ir_lamp.ihx
```

J3Y 带 LED 遥控器：

```sh
cd firmware/remote
pio run -t clean -e STC8H1K08
pio run -e STC8H1K08
pio run -e STC8H1K08 -t upload --upload-port <serial-port>
```

2TY 无 LED 遥控器：

```sh
cd firmware/remote
pio run -t clean -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED
pio run -e STC8H1K08_2TY_NoLED -t upload --upload-port <serial-port>
```

其中 `<stcgal-python>` 是安装了 `stcgal` 的 Python 解释器，`<serial-port>` 是 USB 串口设备，例如 macOS 下的 `/dev/cu.usbserial-110`。STC 下载需要目标板重新上电进入 bootloader。
