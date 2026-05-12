# Stc8hIrLamp

STC8H 红外遥控夜灯产品仓库，包含夜灯固件、遥控器固件和老 Keil 工程完整归档。

## 目录

- `firmware/lamp`: 红外遥控夜灯固件，使用 SDCC + Makefile。
- `firmware/remote`: 红外夜灯遥控器固件，使用 PlatformIO + SDCC。
- `legacy/lamp_keil`: 夜灯老 Keil 工程完整归档。
- `legacy/remote_keil`: 遥控器老 Keil 工程完整归档。
- `docs`: 产品级构建、依赖和归档说明。

## 基础库

本仓库不复制 `Stc8hBase`。默认要求目录布局如下：

```text
<workspace>/
  Stc8hBase/
  Stc8hIrLamp/
```

依赖版本记录见 [docs/dependency.md](docs/dependency.md)。

## 构建

夜灯：

```sh
cd firmware/lamp
make clean && make
```

遥控器：

```sh
cd firmware/remote
pio run -t clean
pio run
```
