# STC8H 红外遥控夜灯

本项目使用同级目录旁的 `Stc8hBase` 基础库重写老项目的红外遥控夜灯功能。

硬件：

- MCU: STC8H1K08。
- LED 驱动: CN5711。
- PWM 输出: P1.0。
- 指示灯: P1.1。
- 红外接收: P3.2 / INT0。

设计文档：

- [需求设计](docs/01_REQUIREMENTS.md)
- [方案设计](docs/02_DESIGN.md)
- [验证记录](docs/03_VERIFICATION.md)

构建：

```sh
make
```

默认开启 UART 调试和低功耗。调试红外链路时可临时关闭低功耗：

```sh
make clean && make POWER_DOWN_ENABLE=0
```

默认基础库路径为 `../../../Stc8hBase`，也可手动覆盖：

```sh
make BASE=../../../Stc8hBase
```
